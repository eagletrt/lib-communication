#pragma once

#include "connection.h"

#include <atomic>
#include <mosquitto.h>

class MQTTConnectionParameters : public ConnectionParameters {
public:
    int port;
    std::string host;
    std::string username;
    std::string password;
    bool tls;
    std::string cafile;
    std::string capath;
    std::string certfile;
    std::string keyfile;

    MQTTConnectionParameters() : ConnectionParameters(){};
    ~MQTTConnectionParameters() override = default;
};

class MQTTMessage : public Message {
public:
    int qos;
    bool retain;
    std::string topic;
    std::string payload;
    std::chrono::system_clock::time_point timestamp;

    MQTTMessage();
    explicit MQTTMessage(const Message &message);
    MQTTMessage(const std::string &topic, const std::string &payload);
    MQTTMessage(const std::string &topic, const std::string &payload, int qos, bool retain);
    ~MQTTMessage() override = default;
};

class MQTTConnection : public Connection {
public:
    explicit MQTTConnection();
    explicit MQTTConnection(const MQTTConnectionParameters &parameters);
    ~MQTTConnection() override;

    void setConnectionParameters(const ConnectionParameters &parameters) override;

    void connect() override;
    void disconnect() override;

    bool send(const Message &message) override;
    void receive(Message &message) override;
    bool queueSend(const Message &message) override;

    void subscribe(const std::string &topic);
    void unsubscribe(const std::string &topic);

    size_t getQueueSize() override;

private:
    static int mqttInstances;
    std::atomic<size_t> queueSize;

    struct mosquitto *mosq;
    MQTTConnectionParameters mqttParameters;

    void loop() override;

    static void on_connect(struct mosquitto *mosq, void *obj, int rc);
    static void on_disconnect(struct mosquitto *mosq, void *obj, int rc);
    static void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);
    static void on_publish(struct mosquitto *mosq, void *obj, int mid);
    static void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);
    static void on_unsubscribe(struct mosquitto *mosq, void *obj, int mid);
};