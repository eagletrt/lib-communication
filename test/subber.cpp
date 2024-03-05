#include "connection.h"
#include "mqtt_connection.h"
#include <chrono>
#include <ctime>
#include <iostream>
#include <unistd.h>

int main() {
    OnConnectCallback onConnectCallback = [](void *data, int id) {
        std::cout << "Connected" << std::endl;
    };
    OnDisconnectCallback onDisconnectCallback = [](void *data, int id) {
        std::cout << "Disconnected" << std::endl;
    };
    OnMessageCallback onMessageCallback = [](void *data, int id, const Message &message) {
        MQTTMessage *msg = (MQTTMessage *) &message;
        std::cout << msg->timestamp.time_since_epoch().count() << " " << msg->topic << ": " << msg->payload << std::endl;
    };
    OnErrorCallback onErrorCallback = [](void *data, int id, const char *error) {
        std::cout << "Error: " << error << std::endl;
    };

    MQTTConnectionParameters parameters;
    parameters.host = "localhost";
    parameters.host = "leonardopivetta.it";
    parameters.port = 1883;
    MQTTConnection connection(parameters);

    Connection *conn = &connection;
    conn->setOnConnectCallback(onConnectCallback);
    conn->setOnDisconnectCallback(onDisconnectCallback);
    conn->setOnMessageCallback(onMessageCallback);
    conn->setOnErrorCallback(onErrorCallback);

    conn->connect();
    connection.subscribe("test_topic");

    while (conn->getStatus() == CONNECTION_STATUS_CONNECTING) {
        std::cout << "Connecting..." << std::endl;
        usleep(100000);
    }

    int count = 0;
    while (true) {
        usleep(5e6);
    }

    return 0;
}
