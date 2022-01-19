#pragma once


#include "net_common.h"
#include "net_message.h"
#include "net_tsqueue.h"
#include "net_connection.h"

namespace bluesoft{
    namespace net{
        template<typename T>
        class server_interface{
        public:

            server_interface(uint16_t port)
            : m_acceptor(m_io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
            {}

            virtual ~server_interface(){
               stop();
            }

            bool start(){
               try {

                   wait_for_client_connection();

                   m_thread_context = std::thread([this](){ m_io_context.run(); });

               }catch (std::exception& ex){
                   std::cerr <<"[SERVER] Exception: " << ex.what() << "\n";
                   return false;
               }

               std::cout << "[SERVER] Started... \n";
                return true;
            }

            void stop(){
                // Request the context to close
                m_io_context.stop();

                // Tidy up the context thread
                if(m_thread_context.joinable()){
                    m_thread_context.join();
                }

                // Inform someone, anybody, if they care...
                std::cout <<"[SERVER] Stopped...\n";
            }

            // ASYNC - Instruct asio to wait for connection
            void wait_for_client_connection(){
                 m_acceptor.async_accept(
                         [this](std::error_code ec, asio::ip::tcp::socket socket){
                             if(!ec){
                                std::cout <<"[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

                                std::shared_ptr<connection<T>> newconn =
                                        std::make_shared<connection<T>>(connection<T>::owner::server,
                                                m_io_context, std::move(socket), m_qmessage_in
                                                );

                                // Give the user server a chance to deny connection
                                if(on_client_connect(newconn)){
                                    // Connection allowed, so add to container of new connection
                                    m_deq_connections.push_back(std::move(newconn));
                                    m_deq_connections.back()->connect_to_client(n_ID_counter++);
                                    std::cout <<"[" << m_deq_connections.back()->get_id() << "] Connection Approved\n";
                                }else{
                                    std::cout << "[------] Connection Denied\n";
                                }

                             } else{
                                 // Error has occurred during acceptance
                                 std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
                             }
                             // Prime the asio context with more work - again simple wait for
                             // another connection...
                             wait_for_client_connection();
                         });
            }

            // Send a message to a specific client
            void message_client(std::shared_ptr<connection<T>> client, const message<T>& msg){
                 if(client && client->is_connected()){
                     client->send(msg);
                 }else{
                     on_client_disconnect(client);
                     client.reset();
                     // Then physically remove it from the container
                     m_deq_connections.erase(
                             std::remove(m_deq_connections.begin(), m_deq_connections.end(), client), m_deq_connections.end());
                 }
            }

            // Send message to all clients
            void message_all_client(const message<T> msg, std::shared_ptr<connection<T>> p_ignore_client = nullptr){

                 bool b_invalid_client_exists = false;

                 for(auto& client : m_deq_connections){
                     // Check client is connected...
                     if(client && client->is_connected()){
                         if(client != p_ignore_client){
                             client->send(msg);
                         }
                     }else{
                         on_client_disconnect(client);
                         client.reset();
                         b_invalid_client_exists = true;
                     }
                 }

                // Remove dead clients, all in one go - this way, we dont invalidate the
                // container as we iterated through it.
                if (b_invalid_client_exists)
                    m_deq_connections.erase(
                            std::remove(m_deq_connections.begin(), m_deq_connections.end(), nullptr), m_deq_connections.end());
            }

            void update(size_t n_max_message = -1){
                size_t  n_message_count = 0;
                while (n_message_count < n_max_message && !m_qmessage_in.empty()){
                    // Grab the front message
                    auto msg = m_qmessage_in.pop_front();

                    // Pass to message handler
                    on_message(msg.remote, msg.msg);
                    n_message_count++;
                }
            }

        protected:

            // Called when a client connects, you can veto the connection by returning false
            virtual bool on_client_connect(std::shared_ptr<connection<T>> client){
                return false;
            }

            // Called when a client appears to have disconnected
            virtual void on_client_disconnect(std::shared_ptr<connection<T>> client){

            }

            // Called when message arives
            virtual void on_message(std::shared_ptr<connection<T>> client, message<T> msg){

            }

        protected:

            // Thread safe queue for incoming message packets
            tsqueue<owned_message<T>> m_qmessage_in;

            // container of active validated connections
            std::deque<std::shared_ptr<connection<T>>> m_deq_connections;

            // Order of declaration is important - it is also the order of initialisation
            asio::io_context m_io_context;
            std::thread m_thread_context;

            // This things need an asio context
            asio::ip::tcp::acceptor m_acceptor;

            // Client will be identified in the "wider system" via an ID
            uint32_t n_ID_counter = 10000;
        };
    }
}