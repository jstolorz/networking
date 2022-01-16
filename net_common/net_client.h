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

            client_interface(): m_socket(m_context){
            }

            virtual ~client_interface(){
                disconnect();
            }

            // Connect to server with hostname/ip-address and port
            bool connect(const std::string& host, const uint16_t port){
                try {
                    // Create connection
                    m_connection = std::make_unique<connection<T>>();

                    // Resolve hostname/ip-address into tangible physical address
                    asio::ip::tcp::resolver resolver(m_context);
                    auto m_endpoints = resolver.resolve(host,std::to_string(port));

                    // Tell the connection object to connect to server
                    m_connection->connect_to_server(m_endpoints);

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

            const tsqueue<owned_message<T>> &incoming() const {
                return m_qmessage_in;
            }

        protected:

            // asio context handles the data transfer...
            asio::io_context m_context;
            // ... but needs a thread of its own to execute its work commands
            std::thread thr_context;
            // This is the hardware socket that is connected to the server
            asio::ip::tcp::socket m_socket;
            // The client has a single instance of a connection object, with handles data transfer
            std::shared_ptr<connection<T>> m_connection;




        private:
            // This is thread safe queue of incoming message from server
            tsqueue<owned_message<T>> m_qmessage_in;
        };
    }
}
