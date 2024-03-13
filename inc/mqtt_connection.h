#pragma once

#include "connection.h"

#include <atomic>
#include <mosquitto.h>

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
  bool will_message_set;
  MQTTMessage will;

	MQTTConnectionParameters() : ConnectionParameters(){};
	~MQTTConnectionParameters() override = default;

  static MQTTConnectionParameters get_default();
};

class MQTTConnection : public Connection {
public:
	explicit MQTTConnection();
	explicit MQTTConnection(const MQTTConnectionParameters &parameters);
	MQTTConnection(MQTTConnection &&);
	MQTTConnection &operator=(MQTTConnection &&);
	~MQTTConnection() override;

	void setConnectionParameters(const ConnectionParameters &parameters) override;
	const MQTTConnectionParameters &getMQTTConnectionParameters() const { return mqttParameters; }

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

  static void throw_error(const int err);
};

class MQTTMessageBuilder {
private:
  MQTTMessage message;
public:
  MQTTMessageBuilder();
  ~MQTTMessageBuilder() = default;

  MQTTMessageBuilder &topic(const std::string &topic);
  MQTTMessageBuilder &payload(const std::string &payload);
  MQTTMessageBuilder &qos(int qos);
  MQTTMessageBuilder &retain(bool retain);

  MQTTMessage build();
};

class MQTTConnectionParametersBuilder {
private:
  MQTTConnectionParameters parameters;
public:
  MQTTConnectionParametersBuilder();
  ~MQTTConnectionParametersBuilder() = default;

  MQTTConnectionParametersBuilder &host(const std::string &host);
  MQTTConnectionParametersBuilder &port(int port);
  MQTTConnectionParametersBuilder &username(const std::string &username);
  MQTTConnectionParametersBuilder &password(const std::string &password);
  MQTTConnectionParametersBuilder &tls(bool tls);
  MQTTConnectionParametersBuilder &cafile(const std::string &cafile);
  MQTTConnectionParametersBuilder &capath(const std::string &capath);
  MQTTConnectionParametersBuilder &certfile(const std::string &certfile);
  MQTTConnectionParametersBuilder &keyfile(const std::string &keyfile);
  MQTTConnectionParametersBuilder &will(const MQTTMessage &will);

  MQTTConnectionParameters build();
};

