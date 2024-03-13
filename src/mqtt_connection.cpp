#include "mqtt_connection.h"
#include <iostream>
#include <mosquitto.h>
#include <mutex>
#include <string>

int MQTTConnection::mqttInstances = 0;

#define MQTT_ERROR(inst, err, message)                         \
  if (inst->onErrorCallback) {                                 \
    char *err_msg = NULL;                                      \
    asprintf(&err_msg, message "%s", mosquitto_strerror(err)); \
    inst->onErrorCallback(inst->userData, inst->id, err_msg);  \
    free(err_msg);                                             \
  }

MQTTMessage::MQTTMessage() : Message() {
  this->qos = 0;
  this->retain = false;
};
MQTTMessage::MQTTMessage(const std::string &topic, const std::string &payload)
    : Message() {
  this->topic = topic;
  this->payload = payload;
  this->retain = false;
  this->qos = 0;
}
MQTTMessage::MQTTMessage(const std::string &topic, const std::string &payload,
                         int qos, bool retain)
    : Message() {
  this->qos = qos;
  this->retain = retain;
  this->topic = topic;
  this->payload = payload;
}
MQTTMessage::MQTTMessage(const Message &message) : Message() {
  if (typeid(message) != typeid(MQTTMessage)) return;
  MQTTMessage *msg = (MQTTMessage *)&message;
  this->qos = msg->qos;
  this->retain = msg->retain;
  this->topic = msg->topic;
  this->payload = msg->payload;
}

void libInit() { mosquitto_lib_init(); }

void libCleanup() { mosquitto_lib_cleanup(); }

MQTTConnection::MQTTConnection() : MQTTConnection(MQTTConnectionParameters()) {}
MQTTConnection::MQTTConnection(const MQTTConnectionParameters &parameters_)
    : Connection(parameters_) {
  if (mqttInstances == 0) libInit();
  mqttInstances++;

  mqttParameters = parameters_;
  parameters = mqttParameters;
  mosq = NULL;
  queueSize.store(0);
}

MQTTConnection::MQTTConnection(MQTTConnection &&other)
    : Connection(std::move(other)),
      mosq(other.mosq),
      mqttParameters(std::move(other.mqttParameters)),
      queueSize(other.queueSize.load()) {
  // Set the moved-from object's mosq to nullptr to prevent double deletion
  other.mosq = nullptr;
}
MQTTConnection &MQTTConnection::operator=(MQTTConnection &&other) {
  if (this != &other) {
    mosq = other.mosq;
    mqttParameters = std::move(other.mqttParameters);
    queueSize = other.queueSize.load();
    other.mosq = nullptr;
  }
  return *this;
}
MQTTConnection::~MQTTConnection() {
  disconnect();
  mqttInstances--;
  if (mqttInstances == 0) libCleanup();
}

void MQTTConnection::setConnectionParameters(
    const ConnectionParameters &parameters_) {
  if (typeid(parameters_) != typeid(MQTTConnectionParameters)) return;
  mqttParameters =
      reinterpret_cast<const MQTTConnectionParameters &>(parameters_);
  parameters = mqttParameters;
}

void MQTTConnection::connect() {
  status = CONNECTION_STATUS_CONNECTING;
  int ret;

  if (mosq) {
    mosquitto_reinitialise(this->mosq, NULL, true, this);
  } else {
    mosq = mosquitto_new(NULL, true, this);
  }

  if (!mosq) {
    status = CONNECTION_STATUS_ERROR;
    MQTT_ERROR(this, MOSQ_ERR_NOMEM, "Error creating mosquitto instance: ")
    return;
  }

  if (!mqttParameters.username.empty() && !mqttParameters.password.empty()) {
    ret = mosquitto_username_pw_set(mosq, mqttParameters.username.c_str(),
                                    mqttParameters.password.c_str());
    if (ret != MOSQ_ERR_SUCCESS) {
      status = CONNECTION_STATUS_ERROR;
      MQTT_ERROR(this, ret, "Error setting username and password: ")
      return;
    }
  }

  if (mqttParameters.tls) {
    ret = mosquitto_tls_set(
        mosq,
        mqttParameters.cafile.empty() ? nullptr : mqttParameters.cafile.c_str(),
        mqttParameters.capath.empty() ? nullptr : mqttParameters.capath.c_str(),
        mqttParameters.certfile.empty() ? nullptr
                                        : mqttParameters.certfile.c_str(),
        mqttParameters.keyfile.empty() ? nullptr
                                       : mqttParameters.keyfile.c_str(),
        nullptr);
    if (ret) {
      status = CONNECTION_STATUS_ERROR;
      MQTT_ERROR(this, ret, "Error setting tls: ")
      return;
    }

    ret = mosquitto_tls_insecure_set(mosq, false);
    if (ret) {
      status = CONNECTION_STATUS_ERROR;
      MQTT_ERROR(this, ret, "Setting TLS: ")
      return;
    }
  }

  if (mqttParameters.will_message_set) {
    ret = mosquitto_will_set(
      mosq,
      mqttParameters.will.topic.c_str(),
      mqttParameters.will.payload.size(),
      mqttParameters.will.payload.c_str(),
      mqttParameters.will.qos,
      mqttParameters.will.retain
    );

    if (ret) {
      status = CONNECTION_STATUS_ERROR;
      MQTT_ERROR(this, ret, "Error setting will message: ")
      return;
    }
  }

	ret = mosquitto_loop_start(mosq);
	if (ret) {
    std::cout << "Error connecting to broker: " << ret << std::endl;
		status = CONNECTION_STATUS_ERROR;
		MQTT_ERROR(this, ret, "Error starting mosquitto loop: ")
		return;
	}

  ret = mosquitto_connect_async(mosq, mqttParameters.host.c_str(),
                                mqttParameters.port, 5);
  if (ret) {
    status = CONNECTION_STATUS_ERROR;
    MQTT_ERROR(this, ret, "Error connecting to broker: ")
    return;
  }

  mosquitto_connect_callback_set(mosq, MQTTConnection::on_connect);
  mosquitto_disconnect_callback_set(mosq, MQTTConnection::on_disconnect);
  mosquitto_message_callback_set(mosq, MQTTConnection::on_message);
  mosquitto_publish_callback_set(mosq, MQTTConnection::on_publish);
  mosquitto_subscribe_callback_set(mosq, MQTTConnection::on_subscribe);
  mosquitto_unsubscribe_callback_set(mosq, MQTTConnection::on_unsubscribe);
}

void MQTTConnection::disconnect() {
	mosquitto_disconnect(mosq);
	mosquitto_loop_stop(mosq, false);
	mosquitto_destroy(mosq);
	mosq = nullptr;
	status = CONNECTION_STATUS_DISCONNECTED;
}

bool MQTTConnection::send(const Message &message) {
  if (typeid(message) != typeid(MQTTMessage)) return false;
  if (queueSize.load() >= maxQueueSize) return false;
  if (!mosq) return false;

  MQTTMessage *mqtt_message = (MQTTMessage *)&message;
  mosquitto_publish(mosq, NULL, mqtt_message->topic.c_str(),
                    mqtt_message->payload.size(), mqtt_message->payload.c_str(),
                    mqtt_message->qos, mqtt_message->retain);
  queueSize++;
  return true;
}

void MQTTConnection::receive(Message &message) {}

bool MQTTConnection::queueSend(const Message &message) { return send(message); }

void MQTTConnection::subscribe(const std::string &topic) {
  mosquitto_subscribe(mosq, NULL, topic.c_str(), 0);
}

void MQTTConnection::unsubscribe(const std::string &topic) {
  mosquitto_unsubscribe(mosq, NULL, topic.c_str());
}

size_t MQTTConnection::getQueueSize() { return queueSize; }

void MQTTConnection::loop() {}

void MQTTConnection::on_connect(struct mosquitto *mosq, void *obj, int rc) {
  MQTTConnection *connection = (MQTTConnection *)obj;
  if (rc == 0) {
    connection->status = CONNECTION_STATUS_CONNECTED;
    if (connection->onConnectCallback)
      connection->onConnectCallback(connection->userData, connection->id);
  } else {
    connection->status = CONNECTION_STATUS_ERROR;
    MQTT_ERROR(connection, rc, "Error on_connect: ")
  }
}

void MQTTConnection::on_disconnect(struct mosquitto *mosq, void *obj, int rc) {
  MQTTConnection *connection = (MQTTConnection *)obj;
  connection->status = CONNECTION_STATUS_DISCONNECTED;
  if (connection->onDisconnectCallback && mosq)
    connection->onDisconnectCallback(connection->userData, connection->id);
}

void MQTTConnection::on_message(struct mosquitto *mosq, void *obj,
                                const struct mosquitto_message *message) {
  MQTTConnection *connection = (MQTTConnection *)obj;
  MQTTMessage mqtt_message;
  mqtt_message.topic = message->topic;
  mqtt_message.payload =
      std::string((char *)message->payload, message->payloadlen);
  mqtt_message.timestamp = std::chrono::system_clock::now();
  if (connection->onMessageCallback)
    connection->onMessageCallback(connection->userData, connection->id,
                                  mqtt_message);
}

void MQTTConnection::on_publish(struct mosquitto *mosq, void *obj, int mid) {
  MQTTConnection *connection = (MQTTConnection *)obj;
  connection->queueSize--;
}

void MQTTConnection::on_subscribe(struct mosquitto *mosq, void *obj, int mid,
                                  int qos_count, const int *granted_qos) {}

void MQTTConnection::on_unsubscribe(struct mosquitto *mosq, void *obj, int mid) {}

MQTTConnectionParametersBuilder::MQTTConnectionParametersBuilder() {
  parameters = MQTTConnectionParameters::get_default();
}

MQTTConnectionParametersBuilder 
&MQTTConnectionParametersBuilder::host(
  const std::string &host
) {
  parameters.host = host;
  return *this;
}

MQTTConnectionParametersBuilder
&MQTTConnectionParametersBuilder::port(
  int port
) {
  parameters.port = port;
  return *this;
}

MQTTConnectionParametersBuilder
&MQTTConnectionParametersBuilder::username(
  const std::string &username
) {
  parameters.username = username;
  return *this;
}

MQTTConnectionParametersBuilder
&MQTTConnectionParametersBuilder::password(
  const std::string &password
) {
  parameters.password = password;
  return *this;
}

MQTTConnectionParametersBuilder
&MQTTConnectionParametersBuilder::cafile(
  const std::string &cafile
) {
  parameters.cafile = cafile;
  return *this;
}

MQTTConnectionParametersBuilder
&MQTTConnectionParametersBuilder::capath(
  const std::string &capath
) {
  parameters.capath = capath;
  return *this;
}

MQTTConnectionParametersBuilder
&MQTTConnectionParametersBuilder::tls(
  bool tls
) {
  parameters.tls = tls;
  return *this;
}

MQTTConnectionParametersBuilder
&MQTTConnectionParametersBuilder::certfile(
  const std::string &certfile
) {
  parameters.certfile = certfile;
  return *this;
}

MQTTConnectionParametersBuilder
&MQTTConnectionParametersBuilder::keyfile(
  const std::string &keyfile
) {
  parameters.keyfile = keyfile;
  return *this;
}

MQTTConnectionParametersBuilder
&MQTTConnectionParametersBuilder::will(
  const MQTTMessage &will
) {
  parameters.will_message_set = true;
  parameters.will = will;
  return *this;
}

MQTTConnectionParameters
MQTTConnectionParametersBuilder::build() {
  return parameters;
}

MQTTConnectionParameters MQTTConnectionParameters::get_default() {
  MQTTConnectionParameters parameters;
  parameters.host = "localhost",
  parameters.port = 1883,
  parameters.tls = false;
  return parameters;
}

MQTTMessageBuilder::MQTTMessageBuilder() {
  message = MQTTMessage();
}

MQTTMessageBuilder& MQTTMessageBuilder::topic(const std::string &topic) {
  message.topic = topic;
  return *this;
}

MQTTMessageBuilder& MQTTMessageBuilder::payload(const std::string &payload) {
  message.payload = payload;
  return *this;
}

MQTTMessageBuilder& MQTTMessageBuilder::qos(int qos) {
  message.qos = qos;
  return *this;
}

MQTTMessageBuilder& MQTTMessageBuilder::retain(bool retain) {
  message.retain = retain;
  return *this;
}

MQTTMessage MQTTMessageBuilder::build() {
  return message;
}

