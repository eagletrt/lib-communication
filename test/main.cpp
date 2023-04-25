#include "connection.h"
#include "mqtt_connection.h"
#include <iostream>
#include <unistd.h>
#include <typeinfo>

int main()
{

    OnConnectCallback onConnectCallback = [](void* data, int id) {
        std::cout << "Connected" << std::endl;
    };
    OnDisconnectCallback onDisconnectCallback = [](void* data, int id) {
        std::cout << "Disconnected" << std::endl;
    };
    OnMessageCallback onMessageCallback = [](void* data, int id, const Message& message) {
        MQTTMessage* msg = (MQTTMessage*)&message;
        std::cout << msg->topic << ": " << msg->payload << std::endl;
    };
    OnErrorCallback onErrorCallback = [](void* data, int id, const char* error) {
        std::cout << "Error: " << error << std::endl;
    };

    MQTTConnectionParameters parameters;
    parameters.host = "167.99.136.159";
    parameters.port = 1883;
    MQTTConnection connection(parameters);

    Connection *conn = &connection;

    while(true){
        conn->setOnConnectCallback(onConnectCallback);
        conn->setOnDisconnectCallback(onDisconnectCallback);
        conn->setOnMessageCallback(onMessageCallback);
        conn->setOnErrorCallback(onErrorCallback);

        conn->connect();

        while(conn->getStatus() == CONNECTING){
            std::cout << "Connecting..." << std::endl;
            usleep(100000);
        }

        if(conn->getStatus() == ERROR || conn->getStatus() == DISCONNECTED){
            std::cout << "Error connecting" << std::endl;
            usleep(5e5);
            continue;
        }

        MQTTMessage msg;
        msg.topic = "update_data";
        msg.payload = "test";
        conn->queueSend(msg);

        connection.send(msg);
        connection.subscribe("update_data");
        connection.send(msg);

        for(int i = 0; i < 5; i++){
            usleep(5e5);
            MQTTMessage msg("update_data", "test");
            connection.send(msg);
        }
        conn->disconnect();
        // parameters.port = 8883;
        // conn->setConnectionParameters(parameters);
        // usleep(5e5);
    }

    return 0;
}
