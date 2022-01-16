
#include "../net_common/bluesoft_net.h"

enum class custom_msg_type :  uint32_t
{
   FireBullet, MovePlayer
};

class custom_client : public bluesoft::net::client_interface<custom_msg_type>{

public:
    bool fire_bullet(float x,float y){
        bluesoft::net::message<custom_msg_type> msg;
        msg.header.id = custom_msg_type::FireBullet;
        msg << x << y;
        //send(msg);
    }

};

int main(){

    custom_client c;
    c.connect("127.0.0.1",99999);
    c.fire_bullet(3.4f,4.5f);

    return 0;
}

