
#include "../net_common/bluesoft_net.h"

enum class custom_msg_types : uint32_t
{
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage
};

class custom_client : public bluesoft::net::client_interface<custom_msg_types>{

public:
    void ping_server() {
        bluesoft::net::message<custom_msg_types> msg;
        msg.header.id = custom_msg_types::ServerPing;

        std::chrono::system_clock::time_point time_now = std::chrono::system_clock::now();

        msg << time_now;
        send(msg);
    }


    void message_all(){
        bluesoft::net::message<custom_msg_types> msg;
        msg.header.id = custom_msg_types::MessageAll;
        send(msg);
    }
};

int main(){

    custom_client c;
    c.connect("127.0.0.1",60000);

    bool key[3] = {false, false,false};
    bool old_key[3] = {false, false,false};

    bool b_quit = false;

    while(!b_quit){

        if(GetForegroundWindow() == GetConsoleWindow()){
            key[0] = GetAsyncKeyState('1') & 0x8000;
            key[1] = GetAsyncKeyState('2') & 0x8000;
            key[2] = GetAsyncKeyState('3') & 0x8000;
        }

        if(key[0] && !old_key[0]){
            c.ping_server();
        }

        if(key[1] && !old_key[1]){
            c.message_all();
        }

        if(key[2] && !old_key[2]){
            b_quit = true;
        }

        for (int i = 0; i < 3; ++i) {
            old_key[i] = key[i];
        }

        if(c.is_connected()){
            if(!c.incoming().empty()){
                auto msg = c.incoming().pop_front().msg;

                switch (msg.header.id) {
                    case custom_msg_types::ServerPing:{
                        std::chrono::system_clock::time_point time_now = std::chrono::system_clock::now();
                        std::chrono::system_clock::time_point time_then;
                        msg >> time_then;
                        std::cout << "Ping: " << std::chrono::duration<double>(time_now - time_then).count() << "\n";
                    }
                    break;

                    case custom_msg_types::ServerMessage:{
                        uint32_t client_id;
                        msg >> client_id;
                        std::cout << "Hello from [" << client_id << "]\n";
                    }
                    break;


                }

            }
        }
    }

    return 0;
}

