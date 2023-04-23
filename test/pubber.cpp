#include "connection.h"
#include "mqtt_connection.h"
#include <iostream>
#include <unistd.h>

int main()
{

    OnConnectCallback onConnectCallback = [](int id) {
        std::cout << "Connected" << std::endl;
    };
    OnDisconnectCallback onDisconnectCallback = [](int id) {
        std::cout << "Disconnected" << std::endl;
    };
    OnMessageCallback onMessageCallback = [](int id, const Message& message) {
        MQTTMessage* msg = (MQTTMessage*)&message;
        std::cout << msg->topic << ": " << msg->payload << std::endl;
    };
    OnErrorCallback onErrorCallback = [](int id, const char* error) {
        std::cout << "Error: " << error << std::endl;
    };

    MQTTConnectionParameters parameters;
    parameters.host = "167.99.136.159";
    parameters.port = 1883;
    MQTTConnection connection(parameters);
    connection.libInit();

    Connection *conn = &connection;
    conn->setOnConnectCallback(onConnectCallback);
    conn->setOnDisconnectCallback(onDisconnectCallback);
    conn->setOnMessageCallback(onMessageCallback);
    conn->setOnErrorCallback(onErrorCallback);

    conn->connect();

    while(conn->getStatus() == CONNECTING){
        std::cout << "Connecting..." << std::endl;
        usleep(100000);
    }

    MQTTMessage msg;
    msg.topic = "update_data";

    int count = 0;
    while(true){
        usleep(1e6);
        msg.payload = "test" + std::to_string(++count);
        conn->send(msg);
    }

    connection.libCleanup();
    return 0;
}
