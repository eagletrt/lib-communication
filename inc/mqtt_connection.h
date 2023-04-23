#pragma once

#include "connection.h"

#include <mosquitto.h>

class MQTTConnectionParameters : public ConnectionParameters
{
public:
  int port;
  std::string host;
  std::string username;
  std::string password;

  MQTTConnectionParameters() : ConnectionParameters() {};
  ~MQTTConnectionParameters() override {};
};

class MQTTMessage : public Message
{
public:
  std::string topic;
  std::string payload;
  std::chrono::_V2::system_clock::time_point timestamp;

  MQTTMessage() : Message() {};
  MQTTMessage(const Message& message) : Message(message) {};
  ~MQTTMessage() override {};
};

class MQTTConnection : public Connection
{
public:
    MQTTConnection(MQTTConnectionParameters& parameters);
    ~MQTTConnection() override;

    void libInit();
    void libCleanup();

    void connect() override;
    void disconnect() override;

    void send(const Message& message) override;
    void receive(Message& message) override;
    void queueSend(const Message& message) override;

    void subscribe(const std::string& topic);
    void unsubscribe(const std::string& topic);

private:
    struct mosquitto* mosq;

    int version_maj, version_min, version_rev;

    MQTTConnectionParameters* mqtt_parameters;

    void loop() override;

    static void on_connect(struct mosquitto* mosq, void* obj, int rc);
    static void on_disconnect(struct mosquitto* mosq, void* obj, int rc);
    static void on_message(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message);
    static void on_publish(struct mosquitto* mosq, void* obj, int mid);
    static void on_subscribe(struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos);
    static void on_unsubscribe(struct mosquitto* mosq, void* obj, int mid);
};