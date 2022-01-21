#pragma once

#include "net_common.h"
#include "net_message.h"
#include "net_tsqueue.h"
#include "net_connection.h"

namespace bluesoft{
    namespace net{

        template<typename T>
        class client_interface{
        public:

            client_interface()
            {}

            virtual ~client_interface(){
                disconnect();
            }

            // Connect to server with hostname/ip-address and port
            bool connect(const std::string& host, const uint16_t port){
                try {

                    asio::ip::tcp::resolver resolver(m_context);
                    asio::ip::tcp::resolver::results_type endpoint = resolver.resolve(host, std::to_string(port));

                    // Create connection
                    m_connection = std::make_unique<connection<T>>(connection<T>::owner::client, m_context,
                                                                   asio::ip::tcp::socket(m_context), m_q_message_in);

                    // Tell the connection object to connect to server
                    m_connection->connect_to_server(endpoint);

                    // Start context thread
                    thr_context = std::thread([this](){ m_context.run(); });


                }catch (std::exception& ex){
                    std::cerr << "Client exception: " << ex.what() << "\n";
                    return false;
                }

                return true;

            }

            // Disconnect from server
            void disconnect(){
                  // If connection exists, and it is connected then...
                  if(is_connected()){
                      // ... disconnect from server
                      m_connection->disconnect();
                  }
            }

            // Check if client is actually connected to the server
            bool is_connected(){
               if(m_connection){
                   return m_connection->is_connected();
               } else{
                   return false;
               }
            }

           void send(const message<T>& msg){
                  if(is_connected()){
                      m_connection->send(msg);
                  }
            }

            // Retrieve queue of messages from server
            tsqueue<owned_message<T>>& incoming(){
                return m_q_message_in;
            }

        protected:

            // asio context handles the data transfer...
            asio::io_context m_context;
            // ... but needs a thread of its own to execute its work commands
            std::thread thr_context;
            // This is the hardware socket that is connected to the server
            //asio::ip::tcp::socket m_socket;
            // The client has a single instance of a connection object, with handles data transfer
            std::shared_ptr<connection<T>> m_connection;




        private:
            // This is thread safe queue of incoming message from server
            tsqueue<owned_message<T>> m_q_message_in;
        };
    }
}
