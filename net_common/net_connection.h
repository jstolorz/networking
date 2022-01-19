#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

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

            void connect_to_client(uint32_t uid = 0){
                if(m_n_owner_type == owner::server){
                    if(m_socket.is_open()){
                        id = uid;
                        read_header();
                    }
                }
            }

            void connect_to_server(const asio::ip::tcp::resolver::results_type& endpoints){
                // Only clients can connect to servers
                if(m_n_owner_type == owner::client){
                    // Request asio attempts to connect to an endpoint
                    asio::async_connect(m_socket, endpoints,
                                        [this](std::error_code ec, asio::ip::tcp::endpoint endpoint){
                        if(!ec){
                            read_header();
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
        };
    }
}