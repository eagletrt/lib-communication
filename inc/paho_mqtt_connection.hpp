#pragma once

#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "mqtt/async_client.h"

class PAHOMQTTConnection;

class PHAOMQTTMessage {
 public:
  PHAOMQTTMessage();
  ~PHAOMQTTMessage() = default;
  PHAOMQTTMessage(const mqtt::message_ptr &msg)
      : PHAOMQTTMessage(msg->get_topic(), msg->to_string(), msg->get_qos(),
                        msg->is_retained()) {};
  PHAOMQTTMessage(const mqtt::const_message_ptr &msg)
      : PHAOMQTTMessage(msg->get_topic(), msg->to_string(), msg->get_qos(),
                        msg->is_retained()) {};
  PHAOMQTTMessage(const PHAOMQTTMessage &other) = default;
  PHAOMQTTMessage(const std::string &topic, const std::string &payload);
  PHAOMQTTMessage(const std::string &topic, const std::string &payload, int qos,
                  bool retain);

  explicit operator mqtt::message_ptr() const {
    mqtt::message_ptr msg = mqtt::make_message(topic, payload);
    msg->set_qos(qos);
    msg->set_retained(retain);
    return msg;
  };

  const std::string &getTopic() const { return topic; };
  const std::string &getPayload() const { return payload; };
  int getQos() const { return qos; };
  bool getRetain() const { return retain; };

 private:
  int qos;
  bool retain;
  std::string topic;
  std::string payload;

  friend class PAHOMQTTConnection;
};

class PAHOMQTTConnectionParameters {
 public:
  PAHOMQTTConnectionParameters();
  ~PAHOMQTTConnectionParameters();

  static PAHOMQTTConnectionParameters get_localhost_default();

  std::string uri;
  std::string username;
  std::string password;
  bool tls;
  std::string cafile;
  std::string capath;
  std::string certfile;
  std::string keyfile;
};

enum class PAHOMQTTConnectionStatus { CONNECTED, CONNECTING, DISCONNECTED };

typedef void (*on_connect_callback)(PAHOMQTTConnection *connection,
                                    void *userData);
typedef void (*on_disconnect_callback)(PAHOMQTTConnection *connection,
                                       void *userData);
typedef void (*on_message_callback)(PAHOMQTTConnection *connection,
                                    void *userData,
                                    const PHAOMQTTMessage &message);
typedef void (*on_error_callback)(PAHOMQTTConnection *connection,
                                  void *userData, const mqtt::token &tok);

class PAHOMQTTConnection : public virtual mqtt::callback,
                           public virtual mqtt::iaction_listener {
 public:
  PAHOMQTTConnection();
  PAHOMQTTConnection(const PAHOMQTTConnectionParameters &parameters);
  ~PAHOMQTTConnection();

  void setConnectionParameters(const PAHOMQTTConnectionParameters &parameters);
  const PAHOMQTTConnectionParameters &getMQTTConnectionParameters() const;

  void connect();
  void disconnect();

  bool send(const PHAOMQTTMessage &message);

  void setWillMessage(const PHAOMQTTMessage &message);
  void disableWillMessage();

  PAHOMQTTConnectionStatus getStatus() const;

  void subscribe(const std::string &topic);
  void unsubscribe(const std::string &topic);
  void unsubscribeAll();
  void subscribeMultiple(const std::vector<std::string> &topics);
  void unsubscribeMultiple(const std::vector<std::string> &topics);

  void setUserData(void *userData);
  void setOnConnectCallback(on_connect_callback callback);
  void setOnDisconnectCallback(on_disconnect_callback callback);
  void setOnMessageCallback(on_message_callback callback);
  void setOnErrorCallback(on_error_callback callback);

 private:
  PAHOMQTTConnectionStatus status;

  PHAOMQTTMessage will;
  PAHOMQTTConnectionParameters mqttParameters;

  std::queue<PHAOMQTTMessage> sendQueue;

  void *userData;
  on_connect_callback onConnectCallback;
  on_disconnect_callback onDisconnectCallback;
  on_message_callback onMessageCallback;
  on_error_callback onErrorCallback;

  // paho
  std::unique_ptr<mqtt::async_client> cli;

 private:
  void on_failure(const mqtt::token &tok) override;
  void on_success(const mqtt::token &tok) override;
  void connected(const std::string &cause) override;
  void connection_lost(const std::string &cause) override;
  void message_arrived(mqtt::const_message_ptr msg) override;
  void delivery_complete(mqtt::delivery_token_ptr token) override;
};