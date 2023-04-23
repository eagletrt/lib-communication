#include "mqtt_connection.h"

#include <typeinfo>
#include <iostream>

MQTTConnection::MQTTConnection(MQTTConnectionParameters& parameters)
: Connection(parameters) {
  this->mqtt_parameters = new MQTTConnectionParameters(parameters);
  this->parameters = this->mqtt_parameters;
  this->mosq = NULL;
}

MQTTConnection::~MQTTConnection() {
  delete this->parameters;
}

void MQTTConnection::libInit() {
  mosquitto_lib_init();
  mosquitto_lib_version(&this->version_maj, &this->version_min, &this->version_rev);
}

void MQTTConnection::libCleanup() {
  mosquitto_lib_cleanup();
}

void MQTTConnection::connect()
{
  if(this->mosq == NULL) {
    this->mosq = mosquitto_new(NULL, true, this);
  } else {
    mosquitto_reinitialise(this->mosq, NULL, true, this);
  }

  mosquitto_loop_start(this->mosq);

  if(this->mosq == NULL) {
    this->status = ERROR;
    this->onErrorCallback(this->id, mosquitto_strerror(errno));
    this->onErrorCallback(this->id, "Could not create mosquitto instance");
    return;
  }

  this->status = CONNECTING;
  if(this->mqtt_parameters->username != "" && this->mqtt_parameters->password != "")
    mosquitto_username_pw_set(this->mosq, this->mqtt_parameters->username.c_str(), this->mqtt_parameters->password.c_str());
  int ret = mosquitto_connect_async(this->mosq, this->mqtt_parameters->host.c_str(), this->mqtt_parameters->port, 60);
  if(ret != MOSQ_ERR_SUCCESS) {
    this->status = ERROR;
    this->onErrorCallback(this->id, "Could not connect to broker");
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
}

void MQTTConnection::send(const Message& message) {
  MQTTMessage* mqtt_message = (MQTTMessage*)&message;
  mosquitto_publish(this->mosq, NULL, mqtt_message->topic.c_str(), mqtt_message->payload.size(), mqtt_message->payload.c_str(), 0, false);
}

void MQTTConnection::receive(Message& message) {
}

void MQTTConnection::queueSend(const Message& message) {
  std::scoped_lock<std::mutex> lock(this->messageQueueMutex);
  this->messageQueue.push(new MQTTMessage(message));
  this->messageQueueCondition.notify_one();
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
    connection->onConnectCallback(connection->id);
  } else {
    connection->status = ERROR;
    connection->onErrorCallback(connection->id, "Could not connect to broker");
  }
}

void MQTTConnection::on_disconnect(struct mosquitto* mosq, void* obj, int rc) {
  MQTTConnection* connection = (MQTTConnection*)obj;
  connection->status = DISCONNECTED;
  connection->onDisconnectCallback(connection->id);
}

void MQTTConnection::on_message(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message) {
  MQTTConnection* connection = (MQTTConnection*)obj;
  MQTTMessage* mqtt_message = new MQTTMessage();
  mqtt_message->topic = message->topic;
  mqtt_message->payload = std::string((char*)message->payload, message->payloadlen);
  mqtt_message->timestamp = std::chrono::system_clock::now();
  connection->onMessageCallback(connection->id, *mqtt_message);
  delete mqtt_message;
}

void MQTTConnection::on_publish(struct mosquitto* mosq, void* obj, int mid) {
}

void MQTTConnection::on_subscribe(struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos) {
}

void MQTTConnection::on_unsubscribe(struct mosquitto* mosq, void* obj, int mid) {
}
