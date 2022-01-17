#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace bluesoft{
    namespace net{

        template<typename T>
        class connection : public  std::enable_shared_from_this<connection<T>>{
        public:

            enum class owner{
                server,
                client
            };

            connection(owner parent, asio::io_context& asio_context,
                       asio::ip::tcp::socket socket,
                       tsqueue<owned_message<T>>& q_in)
                       : m_asio_context(asio_context), m_socket(std::move(socket)),m_qmessage_in(q_in)
            {
                m_nowner_type = parent;
            }

            virtual ~connection(){}

            // This ID is used system wide - its how clients will understand other clients
            // exist across the whole system.
            uint32_t get_id() const
            {
                return id;
            }

        public:

            void connect_to_client(uint32_t uid = 0){
                if(m_nowner_type == owner::server){
                    if(m_socket.is_open()){
                        id = uid;
                    }
                }
            }

            void connect_to_server(const asio::ip::tcp::resolver::results_type& endpoints);
            bool disconnect();
            bool is_connected() const;

        public:
            bool send(const message<T>& msg);

        protected:
            // Each connection has a unique socket to remote
            asio::ip::tcp::socket m_socket;

            // This is shared with the whole asio instance
            asio::io_context& m_asio_context;

            // This queue holds all messages to be sent to the remote side
            // of this connection
            tsqueue<message<T>> m_qmessage_out;

            // This queue holds all messages that have been received from
            // the remote side of this connection. Note it is a reference
            // as the owner of this connection is expected to provide a queue
            tsqueue<owned_message<T>>& m_qmessage_in;

            // The "owner" decides how some of the connection behaves
            owner m_nowner_type = owner::server;

            uint32_t id = 0;
        };
    }
}