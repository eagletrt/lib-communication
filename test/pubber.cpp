#include "connection.h"
#include "connection_manager.h"
#include "mqtt_connection.h"
#include <iostream>
#include <unistd.h>

int main() {

    OnConnectCallback onConnectCallback = [](void *data, int id) {
        std::cout << id << " Connected" << std::endl;
    };
    OnDisconnectCallback onDisconnectCallback = [](void *data, int id) {
        std::cout << id << " Disconnected" << std::endl;
    };
    OnMessageCallback onMessageCallback = [](void *data, int id, const Message &message) {
        MQTTMessage *msg = (MQTTMessage *) &message;
        std::cout << id << " " << msg->topic << ": " << msg->payload << std::endl;
    };
    OnErrorCallback onErrorCallback = [](void *data, int id, const char *error) {
        std::cout << id << " Error: " << error << std::endl;
    };

    MQTTConnectionParameters parameters1;
    parameters1.host = "leonardopivetta.it";
    parameters1.port = 1883;
    MQTTConnection mqtt_conn1(parameters1);

    MQTTConnectionParameters parameters2;
    parameters2.host = "localhost";
    parameters2.port = 1883;
    MQTTConnection mqtt_conn2(parameters2);

    Connection *conn1 = &mqtt_conn1;
    Connection *conn2 = &mqtt_conn2;

    conn1->setOnConnectCallback(onConnectCallback);
    conn2->setOnConnectCallback(onConnectCallback);

    conn1->setOnDisconnectCallback(onDisconnectCallback);
    conn2->setOnDisconnectCallback(onDisconnectCallback);

    conn1->setOnMessageCallback(onMessageCallback);
    conn2->setOnMessageCallback(onMessageCallback);

    conn1->setOnErrorCallback(onErrorCallback);
    conn2->setOnErrorCallback(onErrorCallback);

    std::cout << ConnectionManager::addConnection(conn1) << std::endl;
    std::cout << ConnectionManager::addConnection(conn2) << std::endl;

    ConnectionManager::start();
    usleep(1e5);

    while (conn1->getStatus() == CONNECTION_STATUS_CONNECTING) {
        std::cout << "1 Connecting..." << std::endl;
        usleep(100000);
    }

    while (conn2->getStatus() == CONNECTION_STATUS_CONNECTING) {
        std::cout << "2 Connecting..." << std::endl;
        usleep(100000);
    }

    MQTTMessage msg;
    msg.topic = "test_topic";

    int count = 0;
    while (true) {
        for (int i = 0; i < 1000; i++) {
            msg.payload = "test" + std::to_string(++count);
            std::cout << "1 " << conn1->send(msg) << std::endl;
            std::cout << "2 " << conn2->send(msg) << std::endl;
        }
        usleep(5e6);
    }

    return 0;
}
