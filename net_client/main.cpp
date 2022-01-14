
#include "../net_common/bluesoft_net.h"

enum class custom_msg_type :  uint32_t
{
   FireBullet, MovePlayer
};

int main(){

    bluesoft::net::message<custom_msg_type> msg;
    msg.header.id = custom_msg_type::FireBullet;

    int a{1};
    bool b{true};
    float c{3.1234};

    struct {
        float  x;
        float y;
    }d[5];


    msg << a << b << c << d;

    a = 88;
    b = false;
    c = 99.0f;

    msg >> d >> c >> b >> a;


    return 0;
}

