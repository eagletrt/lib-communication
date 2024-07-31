#include "paho_mqtt_connection.hpp"

#include <assert.h>

#include "mqtt/async_client.h"

int PAHOMQTTConnection::instanceCounter = 0;

PAHOMQTTMessage::PAHOMQTTMessage() : PAHOMQTTMessage("", "", 0, false) {};
PAHOMQTTMessage::PAHOMQTTMessage(const std::string &topic,
                                 const std::string &payload)
    : PAHOMQTTMessage(topic, payload, 0, false) {};
PAHOMQTTMessage::PAHOMQTTMessage(const std::string &topic,
                                 const std::string &payload, int qos,
                                 bool retain)
    : topic(topic), payload(payload), qos(qos), retain(retain) {};

PAHOMQTTConnectionParameters::PAHOMQTTConnectionParameters()
    : uri("mqtt://localhost:1883"),
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
    : PAHOMQTTConnection(
          PAHOMQTTConnectionParameters::get_localhost_default()) {};
PAHOMQTTConnection::PAHOMQTTConnection(
    const PAHOMQTTConnectionParameters &parameters)
    : status(PAHOMQTTConnectionStatus::DISCONNECTED),
      mqttParameters(parameters) {
  instanceCounter++;
  id = instanceCounter;
};
PAHOMQTTConnection::~PAHOMQTTConnection() {};

int PAHOMQTTConnection::getID() const { return id; }

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
  mqtt::create_options createOpts = mqtt::create_options(MQTTVERSION_5);
  createOpts.set_max_buffered_messages(mqttParameters.maxPendingMessages);
  createOpts.set_send_while_disconnected(false);
  cli = std::make_shared<mqtt::async_client>(mqttParameters.uri,
                                             std::to_string(id), createOpts);
  mqtt::connect_options connOpts;
  connOpts.set_clean_session(true);
  connOpts.set_keep_alive_interval(20);
  connOpts.set_automatic_reconnect(true);
  if (!will.topic.empty()) {
    connOpts.set_will_message((mqtt::message_ptr)will);
  }

  cli->set_callback(*this);
  cli->connect(connOpts);
};
void PAHOMQTTConnection::disconnect() {
  cli->disconnect();
  status = PAHOMQTTConnectionStatus::DISCONNECTED;
};

bool PAHOMQTTConnection::send(const PAHOMQTTMessage &message) {
  if (!cli->is_connected()) {
    return false;
  }
  if (cli->get_pending_delivery_tokens().size() >=
      mqttParameters.maxPendingMessages - 1) {
    return false;
  }
  try {
    cli->publish((mqtt::message_ptr)message);
  } catch (const std::exception &e) {
    printf("MQTT: got exception in send: %s", e.what());
    return false;
  }
  return true;
};

void PAHOMQTTConnection::setWillMessage(const PAHOMQTTMessage &message) {
  will = message;
};
void PAHOMQTTConnection::disableWillMessage() { will = PAHOMQTTMessage(); };

void PAHOMQTTConnection::subscribe(const std::string &topic, int qos) {
  cli->subscribe(topic, qos);
}
void PAHOMQTTConnection::unsubscribe(const std::string &topic) {
  cli->unsubscribe(topic);
}

void PAHOMQTTConnection::setUserData(void *userData) {
  this->userData = userData;
}
void PAHOMQTTConnection::setOnConnectCallback(on_connect_callback callback) {
  onConnectCallback = callback;
}
void PAHOMQTTConnection::setOnDisconnectCallback(
    on_disconnect_callback callback) {
  onDisconnectCallback = callback;
}
void PAHOMQTTConnection::setOnMessageCallback(on_message_callback callback) {
  onMessageCallback = callback;
}
void PAHOMQTTConnection::setOnErrorCallback(on_error_callback callback) {
  onErrorCallback = callback;
}

PAHOMQTTConnectionStatus PAHOMQTTConnection::getStatus() const {
  return status;
};
void PAHOMQTTConnection::on_failure(const mqtt::token &tok) {
  if (onErrorCallback) {
    onErrorCallback(this, userData, tok);
  }
};
void PAHOMQTTConnection::on_success(const mqtt::token &tok) {};
void PAHOMQTTConnection::connected(const std::string &cause) {
  status = PAHOMQTTConnectionStatus::CONNECTED;
  if (onConnectCallback) {
    onConnectCallback(this, userData);
  }
};
void PAHOMQTTConnection::connection_lost(const std::string &cause) {
  status = PAHOMQTTConnectionStatus::DISCONNECTED;
  if (onDisconnectCallback) {
    onDisconnectCallback(this, userData);
  }
};
void PAHOMQTTConnection::delivery_complete(mqtt::delivery_token_ptr token) {};
void PAHOMQTTConnection::message_arrived(mqtt::const_message_ptr msg) {
  if (onMessageCallback) {
    onMessageCallback(this, userData, PAHOMQTTMessage(msg));
  }
};
