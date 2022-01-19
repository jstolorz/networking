#include <iostream>
#include "../net_common/bluesoft_net.h"

enum class custom_msg_types : uint32_t {
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage
};

class custom_server: public bluesoft::net::server_interface<custom_msg_types>{
public:
    custom_server(uint16_t port) : bluesoft::net::server_interface<custom_msg_types>(port)
    {}

protected:
    virtual bool on_client_connect(std::shared_ptr<bluesoft::net::connection<custom_msg_types>> client){
        bluesoft::net::message<custom_msg_types> msg;
        msg.header.id = custom_msg_types::ServerAccept;
        client->send(msg);
        return true;
    }

    virtual void on_client_disconnect(std::shared_ptr<bluesoft::net::connection<custom_msg_types>> client){
        std::cout << "Removing client [" << client->get_id() << "]\n";
    }

    virtual void on_message(std::shared_ptr<bluesoft::net::connection<custom_msg_types>> client, bluesoft::net::message<custom_msg_types> msg){
        switch (msg.header.id) {
            case custom_msg_types::ServerPing:{
                std::cout << "[" << client->get_id() << "]: Server Ping\n";
                client->send(msg);
            }
                break;
            case custom_msg_types::MessageAll:{
                std::cout << "[" << client->get_id() << "]: Message All\n";
                bluesoft::net::message<custom_msg_types> msg;
                msg.header.id = custom_msg_types::ServerMessage;
                msg << client->get_id();
                message_all_client(msg,client);
            }
                break;
        }
    }
};

int main(){

    custom_server server(60000);
    server.start();

    while (1){
        server.update(-1, true);
    }

    return 0;
}
