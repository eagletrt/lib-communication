#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "mqtt/async_client.h"

class PAHOMQTTConnection;

class PAHOMQTTMessage {
 public:
  PAHOMQTTMessage();
  ~PAHOMQTTMessage() = default;
  PAHOMQTTMessage(const mqtt::message_ptr &msg)
      : PAHOMQTTMessage(msg->get_topic(), msg->to_string(), msg->get_qos(), msg->is_retained()) {};
  PAHOMQTTMessage(const mqtt::const_message_ptr &msg)
      : PAHOMQTTMessage(msg->get_topic(), msg->to_string(), msg->get_qos(), msg->is_retained()) {};
  PAHOMQTTMessage(const PAHOMQTTMessage &other) = default;
  PAHOMQTTMessage(const std::string &topic, const std::string &payload);
  PAHOMQTTMessage(const std::string &topic, const std::string &payload, int qos, bool retain);

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

  size_t maxPendingMessages = 10;
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

typedef void (*on_connect_callback)(PAHOMQTTConnection *connection, void *userData);
typedef void (*on_disconnect_callback)(PAHOMQTTConnection *connection, void *userData);
typedef void (*on_message_callback)(PAHOMQTTConnection *connection, void *userData, const PAHOMQTTMessage &message);
typedef void (*on_error_callback)(PAHOMQTTConnection *connection, void *userData, const mqtt::token &tok);

class PAHOMQTTConnection : public virtual mqtt::callback, public virtual mqtt::iaction_listener {
 public:
  PAHOMQTTConnection();
  PAHOMQTTConnection(const PAHOMQTTConnectionParameters &parameters);
  PAHOMQTTConnection(const PAHOMQTTConnection &other) = delete;
  ~PAHOMQTTConnection();

  int getID() const;

  void setConnectionParameters(const PAHOMQTTConnectionParameters &parameters);
  const PAHOMQTTConnectionParameters &getMQTTConnectionParameters() const;

  void connect();
  void disconnect();

  bool send(const PAHOMQTTMessage &message);

  void setWillMessage(const PAHOMQTTMessage &message);
  void disableWillMessage();

  PAHOMQTTConnectionStatus getStatus() const;

  // Whether the broker has stopped answering a connection the socket still
  // reports as up: a message published at QoS 1 or 2 has waited longer than
  // `timeout` for its acknowledgement. paho's keepalive cannot see this for a
  // client that keeps publishing, since it only pings after keepAlive seconds
  // with nothing sent, and a link that died without a reset (an address change
  // on wifi or 4G, a NAT dropping the flow) then looks connected until TCP
  // gives up, a quarter of an hour later on Linux. Only as good as the QoS 1
  // traffic the owner sends: with none, it never reports a stall.
  bool isStalled(std::chrono::milliseconds timeout) const;

  // Drops the current client and connects a new one, whatever the status says.
  // For a stalled connection: the new client is created before the old one is
  // released, so paho's shared threads are not torn down and rebuilt between
  // the two, which disconnect() would do for the last client left.
  void restart();

  void subscribe(const std::string &topic, int qos = 0);
  void unsubscribe(const std::string &topic);

  void setUserData(void *userData);
  void setOnConnectCallback(on_connect_callback callback);
  void setOnDisconnectCallback(on_disconnect_callback callback);
  void setOnMessageCallback(on_message_callback callback);
  void setOnErrorCallback(on_error_callback callback);

 private:
  int id;
  static int instanceCounter;
  std::atomic<PAHOMQTTConnectionStatus> status;

  // Oldest QoS 1/2 publish still waiting for the broker, in steady-clock
  // milliseconds; cleared by any acknowledgement. Written by send() and by
  // paho's callback thread.
  std::atomic<bool> awaitingAck{false};
  std::atomic<int64_t> awaitingAckSinceMs{0};

  PAHOMQTTMessage will;
  PAHOMQTTConnectionParameters mqttParameters;

  std::queue<PAHOMQTTMessage> sendQueue;

  // Null until set: they are only tested for null before the call, and paho
  // calls connection_lost on its own thread whether or not the owner set one.
  void *userData = nullptr;
  on_connect_callback onConnectCallback = nullptr;
  on_disconnect_callback onDisconnectCallback = nullptr;
  on_message_callback onMessageCallback = nullptr;
  on_error_callback onErrorCallback = nullptr;

  // paho
  std::shared_ptr<mqtt::async_client> cli;

 private:
  void on_failure(const mqtt::token &tok) override;
  void on_success(const mqtt::token &tok) override;
  void connected(const std::string &cause) override;
  void connection_lost(const std::string &cause) override;
  void message_arrived(mqtt::const_message_ptr msg) override;
  void delivery_complete(mqtt::delivery_token_ptr token) override;

  void on_disconnect(const mqtt::properties &, mqtt::ReasonCode);
};
