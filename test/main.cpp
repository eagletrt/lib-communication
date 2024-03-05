#include "connection.h"
#include "mqtt_connection.h"
#include <iostream>
#include <typeinfo>
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
        std::cout << msg->topic << ": " << msg->payload << std::endl;
    };
    OnErrorCallback onErrorCallback = [](void *data, int id, const char *error) {
        std::cout << "Error: " << error << std::endl;
    };

    MQTTConnectionParameters parameters = MQTTConnectionParametersBuilder()
        .host("localhost")
        // .host("broker.emqx.io")
        // .port(1883)
        // .tls(false)
        .will(MQTTMessageBuilder()
            .topic("sesso")
            .payload("gay")
            // .qos(1)
            // .retain(true)
            .build()
        )
        .build();

    MQTTConnection connection(parameters);

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
        conn->disconnect();

        // MQTTMessage msg;
        // msg.topic = "sesso";
        // msg.payload = "anale";
        // conn->queueSend(msg);
        //
        // connection.send(msg);
        // connection.subscribe("sesso");
        // connection.send(msg);
        //
        // for (int i = 0; i < 5; i++) {
        //     usleep(5e5);
        //     MQTTMessage msg("update_data", "test");
        //     connection.send(msg);
        // }
        // conn->disconnect();
        // parameters.port = 8883;
        // conn->setConnectionParameters(parameters);
        // usleep(5e5);
    }

    return 0;
}
