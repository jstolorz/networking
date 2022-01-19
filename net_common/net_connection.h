#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_client.h"
#include "net_server.h"


namespace bluesoft{
    namespace net{

        template<typename T>
        class connection : public  std::enable_shared_from_this<connection<T>>{
        public:

            // A connection is "owned" by either a server or a client, and its
            // behaviour is slightly different bewteen the two.
            enum class owner
            {
                server,
                client
            };

        public:
            // Constructor: Specify Owner, connect to context, transfer the socket
            //				Provide reference to incoming message queue
            connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& qIn)
                    : m_asio_context(asioContext), m_socket(std::move(socket)), m_q_message_in(qIn)
            {
                m_n_owner_type = parent;

                // Construct validation check data
                if(m_n_owner_type = owner::server){
                   // connection is Server -> client, construct random data for the client
                   // to transform and send back for validation
                   m_n_handshake_out = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

                   // Pre-calculate the result for checking when the client response
                   m_n_handshake_check = scramble(m_q_message_out);
                }else{
                      m_n_handshake_in = 0;
                      m_n_handshake_out = 0;
                }
            }

            virtual ~connection()
            {}

            // This ID is used system wide - its how clients will understand other clients
            // exist across the whole system.
            uint32_t get_id() const
            {
                return id;
            }

        public:


            void connect_to_client(bluesoft::net::server_interface<T>* server, uint32_t uid = 0){
                if(m_n_owner_type == owner::server){
                    if(m_socket.is_open()){
                        id = uid;
                        // read_header();

                        write_validation();

                        read_validation(server);
                    }
                }
            }

            // ASYNC - Used by both client and server to write validation packet
            void write_validation() {
                asio::async_write(m_socket, asio::buffer(&m_n_handshake_out, sizeof(uint64_t)),
                                  [this](std::error_code ec, std::size_t length){
                    if(!ec){

                        // Validation data sent, client should sit and wait
                        // for a response (or close)
                        if(m_n_owner_type == owner::client){
                            read_header();
                        }
                    }else{
                        m_socket.close();
                    }
                });
            }


            void read_validation(bluesoft::net::server_interface<T>* server = nullptr) {
                  asio::async_read(m_socket, asio::buffer(&m_n_handshake_in, sizeof(uint64_t)),
                                   [this,server](std::error_code ec, std::size_t length){
                      if(!ec){
                          if(m_n_owner_type == owner::server){
                              if(m_n_handshake_in == m_n_handshake_check){
                                  std::cout << "client Validated \n";
                                  server->on_client_validated(this->shared_from_this());
                                  read_header();
                              }else{
                                  std::cout << "Client Disconnected (Fail Validation)\n";
                                  m_socket.close();
                              }
                          }else{
                              m_n_handshake_out = scramble(m_n_handshake_in);
                              write_validation();
                          }
                      }else{
                          std::cout << "Client Disconnected (Read Validation) \n";
                          m_socket.close();
                      }
                  });
            }

            void connect_to_server(const asio::ip::tcp::resolver::results_type& endpoints){
                // Only clients can connect to servers
                if(m_n_owner_type == owner::client){
                    // Request asio attempts to connect to an endpoint
                    asio::async_connect(m_socket, endpoints,
                                        [this](std::error_code ec, asio::ip::tcp::endpoint endpoint){
                        if(!ec){

                            // Was: read_header();

                            // First thing server will do is send packet to be validated
                            //  so wait for that and response
                            read_validation();

                        }
                    });
                }
            }

            void disconnect(){
                if(is_connected()){
                    asio::post(m_asio_context,[this](){ m_socket.close(); });
                }
            }

            bool is_connected() const{
                return m_socket.is_open();
            }

        public:
            void send(const message<T>& msg){
                asio::post(m_asio_context,[this,msg](){
                    bool b_write_message = !m_q_message_out.empty();
                    m_q_message_out.push_back(msg);
                    if(!b_write_message){
                        write_header();
                    }
                });
            }

        private:
            // Once a full message is received, add it to the incoming queue
            void add_to_incoming_message_queue() {
               if(m_n_owner_type == owner::server){
                   m_q_message_in.push_back({this->shared_from_this(), m_msg_temporary_in});
               }else{
                   m_q_message_in.push_back({nullptr, m_msg_temporary_in});
               }

               read_header();
            }

// ASYNC - Prime context ready to read a message header
            void read_header(){
                asio::async_read(m_socket, asio::buffer(&m_msg_temporary_in.header, sizeof(message_header<T>)),
                                 [this](std::error_code ec, std::size_t length){
                    if(!ec){

                        if(m_msg_temporary_in.header.size > 0){
                            m_msg_temporary_in.body.resize(m_msg_temporary_in.header.size);
                            read_body();
                        }else{
                           add_to_incoming_message_queue();
                        }

                    } else{
                        std::cout << "[" << id << "] Read Header Fail.\n";
                        m_socket.close();
                    }

                });
            }

            // ASYNC - Prime context ready to read a message body
            void read_body(){

                asio::async_read(m_socket, asio::buffer(m_msg_temporary_in.body.data(), m_msg_temporary_in.body.size()),
                                                        [this](std::error_code ec, std::size_t length){
                    if(!ec){
                        add_to_incoming_message_queue();
                    }else{
                        std::cout << "[" << id << "] Read Body Fail.\n";
                        m_socket.close();
                    }
                });


            }

            // ASYNC - Prime context ready to write a message header
            void write_header(){

                asio::async_write(m_socket, asio::buffer(&m_q_message_out.front().header, sizeof(message_header<T>)),
                                  [this](std::error_code ec, std::size_t length){
                    if(!ec){
                         if(m_q_message_out.front().body.size() > 0){
                             write_body();
                         }else{
                             m_q_message_out.pop_front();
                             if(!m_q_message_out.empty()){
                                 write_header();
                             }
                         }

                    }else{
                        std::cout << "[" << id << "] Write Header Fail.\n";
                        m_socket.close();
                    }
                });

            }

            // ASYNC - Prime context ready to write a message body
            void write_body(){
                asio::async_write(m_socket, asio::buffer(m_q_message_out.front().body.data(),
                                                         m_q_message_out.front().body.size()),
                                  [this](std::error_code ec, std::size_t length){
                     if(!ec){
                         m_q_message_out.pop_front();

                         if(!m_q_message_out.empty()){
                             write_header();
                         }
                     }else{
                         // Sending failed, see WriteHeader() equivalent for description :P
                         std::cout << "[" << id << "] Write Body Fail.\n";
                         m_socket.close();
                     }
                });
            };

            // encrypt data
            uint64_t scramble(uint64_t n_input){
                uint64_t out = n_input ^ 0xDEADBEEFC0DECAFE;
                out  = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0xF0F0F0F0F0F0F0) << 4;
                return out ^ 0xC0DEFACE12345678;
            }

        protected:
            // Each connection has a unique socket to remote
            asio::ip::tcp::socket m_socket;

            // This is shared with the whole asio instance
            asio::io_context& m_asio_context;

            // This queue holds all messages to be sent to the remote side
            // of this connection
            tsqueue<message<T>> m_q_message_out;

            // This queue holds all messages that have been received from
            // the remote side of this connection. Note it is a reference
            // as the owner of this connection is expected to provide a queue
            tsqueue<owned_message<T>>& m_q_message_in;

            // Incoming messages are constructed asynchronously, so we will
            // store the part assembled message here, until it is ready
            message<T> m_msg_temporary_in;


            // The "owner" decides how some connection behaves
            owner m_n_owner_type = owner::server;

            uint32_t id = 0;

            uint64_t m_n_handshake_out = 0;
            uint64_t m_n_handshake_in = 0;
            uint64_t m_n_handshake_check = 0;
        };
    }
}