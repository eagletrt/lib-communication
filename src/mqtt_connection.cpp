#include "mqtt_connection.h"

#include <typeinfo>
#include <iostream>

int MQTTConnection::mqttInstances = 0;

MQTTMessage::MQTTMessage() : Message() {
  this->qos = 0;
  this->retain = false;
};
MQTTMessage::MQTTMessage(const std::string &topic, const std::string &payload) : Message() {
  this->topic = topic;
  this->payload = payload;
  this->retain = false;
  this->qos = 0;
}
MQTTMessage::MQTTMessage(const std::string &topic, const std::string &payload, int qos, bool retain) : Message() {
  this->qos = qos;
  this->retain = retain;
  this->topic = topic;
  this->payload = payload;
}
MQTTMessage::MQTTMessage(const Message& message) : Message() {
  if(typeid(message) != typeid(MQTTMessage))
    return;
  MQTTMessage* msg = (MQTTMessage*)&message;
  this->qos = msg->qos;
  this->retain = msg->retain;
  this->topic = msg->topic;
  this->payload = msg->payload;
}

void libInit() {
  mosquitto_lib_init();
}

void libCleanup() {
  mosquitto_lib_cleanup();
}

MQTTConnection::MQTTConnection(MQTTConnectionParameters& parameters)
: Connection(parameters) {
  if(mqttInstances == 0)
    libInit();
  mqttInstances ++;

  this->mqttParameters = new MQTTConnectionParameters(parameters);
  this->parameters = this->mqttParameters;
  this->mosq = NULL;
  this->queueSize.store(0);
}

MQTTConnection::~MQTTConnection() {
  mqttInstances --;
  if(mqttInstances == 0)
    libCleanup();
  delete this->parameters;
}

void MQTTConnection::setConnectionParameters(ConnectionParameters& parameters){
  if(typeid(parameters) != typeid(MQTTConnectionParameters))
    return;
  *this->mqttParameters = reinterpret_cast<MQTTConnectionParameters&>(parameters);
  this->parameters = this->mqttParameters;
}

void MQTTConnection::connect() {
  if(this->mosq == NULL) {
    this->mosq = mosquitto_new(NULL, true, this);
  } else {
    mosquitto_reinitialise(this->mosq, NULL, true, this);
  }

  mosquitto_loop_start(this->mosq);

  if(this->mosq == NULL) {
    this->status = ERROR;
    this->onErrorCallback(this->userData, this->id, mosquitto_strerror(errno));
    this->onErrorCallback(this->userData, this->id, "Could not create mosquitto instance");
    return;
  }

  this->status = CONNECTING;
  if(this->mqttParameters->username != "" && this->mqttParameters->password != "")
    mosquitto_username_pw_set(this->mosq, this->mqttParameters->username.c_str(), this->mqttParameters->password.c_str());
  int ret = mosquitto_connect_async(this->mosq, this->mqttParameters->host.c_str(), this->mqttParameters->port, 60);
  if(ret != MOSQ_ERR_SUCCESS) {
    this->status = ERROR;
    this->onErrorCallback(this->userData, this->id, "Could not connect to broker");
    return;
  }

  mosquitto_connect_callback_set(this->mosq, MQTTConnection::on_connect);
  mosquitto_disconnect_callback_set(this->mosq, MQTTConnection::on_disconnect);
  mosquitto_message_callback_set(this->mosq, MQTTConnection::on_message);
  mosquitto_publish_callback_set(this->mosq, MQTTConnection::on_publish);
  mosquitto_subscribe_callback_set(this->mosq, MQTTConnection::on_subscribe);
  mosquitto_unsubscribe_callback_set(this->mosq, MQTTConnection::on_unsubscribe);
}

void MQTTConnection::disconnect() {
  mosquitto_disconnect(this->mosq);
  mosquitto_loop_stop(this->mosq, false);
  mosquitto_destroy(this->mosq);
  this->mosq = NULL;
  this->status = DISCONNECTED;
}

void MQTTConnection::send(const Message& message) {
  MQTTMessage* mqtt_message = (MQTTMessage*)&message;
  mosquitto_publish(
    this->mosq,
    NULL,
    mqtt_message->topic.c_str(),
    mqtt_message->payload.size(),
    mqtt_message->payload.c_str(),
    mqtt_message->qos,
    mqtt_message->retain
  );
  this->queueSize ++;
}

void MQTTConnection::receive(Message& message) {
}

void MQTTConnection::queueSend(const Message& message) {
  send(message);
}

void MQTTConnection::subscribe(const std::string& topic) {
  mosquitto_subscribe(this->mosq, NULL, topic.c_str(), 0);
}

void MQTTConnection::unsubscribe(const std::string& topic) {
  mosquitto_unsubscribe(this->mosq, NULL, topic.c_str());
}

void MQTTConnection::loop(){
}

void MQTTConnection::on_connect(struct mosquitto* mosq, void* obj, int rc) {
  MQTTConnection* connection = (MQTTConnection*)obj;
  if(rc == 0) {
    connection->status = CONNECTED;
    connection->onConnectCallback(connection->userData, connection->id);
  } else {
    connection->status = ERROR;
    connection->onErrorCallback(connection->userData, connection->id, "Could not connect to broker");
  }
}

void MQTTConnection::on_disconnect(struct mosquitto* mosq, void* obj, int rc) {
  MQTTConnection* connection = (MQTTConnection*)obj;
  connection->status = DISCONNECTED;
  connection->onDisconnectCallback(connection->userData, connection->id);
}

void MQTTConnection::on_message(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message) {
  MQTTConnection* connection = (MQTTConnection*)obj;
  MQTTMessage* mqtt_message = new MQTTMessage();
  mqtt_message->topic = message->topic;
  mqtt_message->payload = std::string((char*)message->payload, message->payloadlen);
  mqtt_message->timestamp = std::chrono::system_clock::now();
  connection->onMessageCallback(connection->userData, connection->id, *mqtt_message);
  delete mqtt_message;
}

void MQTTConnection::on_publish(struct mosquitto* mosq, void* obj, int mid) {
  MQTTConnection* connection = (MQTTConnection*)obj;
  connection->queueSize --;
}

void MQTTConnection::on_subscribe(struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos) {
}

void MQTTConnection::on_unsubscribe(struct mosquitto* mosq, void* obj, int mid) {
}
