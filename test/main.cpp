#include "connection.h"
#include "mqtt_connection.h"
#include <iostream>
#include <typeinfo>
#include <unistd.h>

int main() {

    OnConnectCallback onConnectCallback = [](void *data, int id) {
        std::cout << "Connected" << std::endl;
        MQTTConnection *conn = (MQTTConnection *) data;

        conn->send(MQTTMessageBuilder()
            .topic("$status")
            .payload("connected")
            .build()
        );
    };
    OnDisconnectCallback onDisconnectCallback = [](void *data, int id) {
        std::cout << "Disconnected" << std::endl;
    };
    OnMessageCallback onMessageCallback = [](void *data, int id, const Message &message) {
        MQTTMessage *msg = (MQTTMessage *) &message;
        std::cout << msg->topic << ": " << msg->payload << std::endl;
    };
    OnErrorCallback onErrorCallback = [](void *data, int id, const char *error) {
        std::cout << "Error: " << error << std::endl;
    };

    MQTTConnectionParameters parameters = MQTTConnectionParametersBuilder()
        .host("localhost")
        .will(MQTTMessageBuilder()
            .topic("$status")
            .payload("crashed")
            .build()
        )
        .build();

    MQTTConnection connection(parameters);
    connection.setUserData(&connection);

    Connection *conn = &connection;

    conn->setOnConnectCallback(onConnectCallback);
    conn->setOnDisconnectCallback(onDisconnectCallback);
    conn->setOnMessageCallback(onMessageCallback);
    conn->setOnErrorCallback(onErrorCallback);

    while (true) {

        conn->connect();

        while (conn->getStatus() == ConnectionStatus::CONNECTION_STATUS_CONNECTING) {
            std::cout << "Connecting..." << std::endl;
            usleep(100000);
        }

        std::cout << "Connected (?)" << std::endl;

        if (conn->getStatus() == ConnectionStatus::CONNECTION_STATUS_ERROR || 
            conn->getStatus() == ConnectionStatus::CONNECTION_STATUS_DISCONNECTED) {

            std::cout << "Error connecting: " << conn->getStatus() << std::endl;
            usleep(5e5);
            continue;
        }

        sleep(5);

        conn->send(MQTTMessageBuilder()
            .topic("$status")
            .payload("disconnected")
            .build()
        );

        conn->disconnect();
    }

    return 0;
}
