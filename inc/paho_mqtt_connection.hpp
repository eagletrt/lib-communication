#pragma once

#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class PHAOMQTTMessage {
 public:
  PHAOMQTTMessage();
  PHAOMQTTMessage(const PHAOMQTTMessage &other) = default;
  PHAOMQTTMessage(const std::string &topic, const std::string &payload);
  PHAOMQTTMessage(const std::string &topic, const std::string &payload, int qos,
                  bool retain);

 private:
  int qos;
  bool retain;
  std::string topic;
  std::string payload;
};

class PAHOMQTTConnectionParameters {
 public:
  PAHOMQTTConnectionParameters();
  ~PAHOMQTTConnectionParameters();

  static PAHOMQTTConnectionParameters get_localhost_default();

 private:
  int port;
  std::string host;
  std::string username;
  std::string password;
  bool tls;
  std::string cafile;
  std::string capath;
  std::string certfile;
  std::string keyfile;
};

enum class PAHOMQTTConnectionStatus { CONNECTED, CONNECTING, DISCONNECTED };

class PAHOMQTTConnection;
typedef void (*on_connect_callback)(PAHOMQTTConnection *connection,
                                    void *userData);
typedef void (*on_disconnect_callback)(PAHOMQTTConnection *connection,
                                       void *userData);
typedef void (*on_message_callback)(PAHOMQTTConnection *connection,
                                    const PHAOMQTTMessage &message,
                                    void *userData);

class PAHOMQTTConnection {
 public:
  PAHOMQTTConnection();
  PAHOMQTTConnection(const PAHOMQTTConnectionParameters &parameters);
  ~PAHOMQTTConnection();

  void setConnectionParameters(const PAHOMQTTConnectionParameters &parameters);
  const PAHOMQTTConnectionParameters &getMQTTConnectionParameters() const;

  void connect();
  void disconnect();

  bool queueSend(const PHAOMQTTMessage &message);

  void setWillMessage(const PHAOMQTTMessage &message);
  void disableWillMessage();

  PAHOMQTTConnectionStatus getStatus() const;
  size_t getQueueSize();

  void subscribe(const std::string &topic);
  void unsubscribe(const std::string &topic);
  void unsubscribeAll();
  void subscribeMultiple(const std::vector<std::string> &topics);
  void unsubscribeMultiple(const std::vector<std::string> &topics);

  void setUserData(void *userData);
  void setOnConnectCallback(on_connect_callback callback);
  void setOnDisconnectCallback(on_disconnect_callback callback);
  void setOnMessageCallback(on_message_callback callback);

 private:
  PAHOMQTTConnectionStatus status;

  bool willMessageSet;
  PHAOMQTTMessage will;
  PAHOMQTTConnectionParameters mqttParameters;

  std::queue<PHAOMQTTMessage> sendQueue;

  void *userData;
  on_connect_callback onConnectCallback;
  on_disconnect_callback onDisconnectCallback;
  on_message_callback onMessageCallback;
};