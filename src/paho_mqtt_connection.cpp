#include "paho_mqtt_connection.hpp"

#include "mqtt/async_client.h"

PHAOMQTTMessage::PHAOMQTTMessage() : PHAOMQTTMessage("", "", 0, false) {};
PHAOMQTTMessage::PHAOMQTTMessage(const std::string &topic,
                                 const std::string &payload)
    : PHAOMQTTMessage(topic, payload, 0, false) {};
PHAOMQTTMessage::PHAOMQTTMessage(const std::string &topic,
                                 const std::string &payload, int qos,
                                 bool retain)
    : topic(topic), payload(payload), qos(qos), retain(retain) {};

PAHOMQTTConnectionParameters::PAHOMQTTConnectionParameters()
    : port(1883),
      host("localhost"),
      username(""),
      password(""),
      tls(false),
      cafile(""),
      capath(""),
      certfile(""),
      keyfile("") {};

PAHOMQTTConnectionParameters::~PAHOMQTTConnectionParameters() {};

PAHOMQTTConnectionParameters
PAHOMQTTConnectionParameters::get_localhost_default() {
  return PAHOMQTTConnectionParameters();
};

PAHOMQTTConnection::PAHOMQTTConnection()
    : status(PAHOMQTTConnectionStatus::DISCONNECTED),
      mqttParameters(PAHOMQTTConnectionParameters()) {};
PAHOMQTTConnection::PAHOMQTTConnection(
    const PAHOMQTTConnectionParameters &parameters)
    : status(PAHOMQTTConnectionStatus::DISCONNECTED),
      mqttParameters(parameters) {};
PAHOMQTTConnection::~PAHOMQTTConnection() {};

void PAHOMQTTConnection::setConnectionParameters(
    const PAHOMQTTConnectionParameters &parameters) {
  mqttParameters = parameters;
};
const PAHOMQTTConnectionParameters &
PAHOMQTTConnection::getMQTTConnectionParameters() const {
  return mqttParameters;
};

void PAHOMQTTConnection::connect() {
  status = PAHOMQTTConnectionStatus::CONNECTING;
  // Connect to MQTT broker
  status = PAHOMQTTConnectionStatus::CONNECTED;
};
void PAHOMQTTConnection::disconnect() {
  status = PAHOMQTTConnectionStatus::DISCONNECTED;
  // Disconnect from MQTT broker
};

bool PAHOMQTTConnection::queueSend(const PHAOMQTTMessage &message) {
  // Send message to MQTT broker
  return true;
};

void PAHOMQTTConnection::setWillMessage(const PHAOMQTTMessage &message) {
  will = message;
  willMessageSet = true;
};
void PAHOMQTTConnection::disableWillMessage() {
  will = PHAOMQTTMessage();
  willMessageSet = false;
};

void PAHOMQTTConnection::subscribe(const std::string &topic) {}
void PAHOMQTTConnection::unsubscribe(const std::string &topic) {}
void PAHOMQTTConnection::unsubscribeAll() {}
void PAHOMQTTConnection::subscribeMultiple(
    const std::vector<std::string> &topics) {}
void PAHOMQTTConnection::unsubscribeMultiple(
    const std::vector<std::string> &topics) {}

void PAHOMQTTConnection::setUserData(void *userData) {}
void PAHOMQTTConnection::setOnConnectCallback(on_connect_callback callback) {}
void PAHOMQTTConnection::setOnDisconnectCallback(
    on_disconnect_callback callback) {}
void PAHOMQTTConnection::setOnMessageCallback(on_message_callback callback) {}