#include "ws_connection.h"
#include "connection.h"
#include "mqtt_connection.h"
#include <iostream>


int main()
{
    MQTTConnection tmp("sube", "melearadio");
    std::cout<<tmp.getPendingMessages()<<"\n";
    return 0;
}
