#include "mqtt_connection.h"

#include <iostream>
#include <typeinfo>

int MQTTConnection::mqttInstances = 0;

#define MQTT_ERROR(inst, err, message)                             \
    if (inst->onErrorCallback) {                                   \
        char *err_msg = NULL;                                      \
        asprintf(&err_msg, message "%s", mosquitto_strerror(err)); \
        inst->onErrorCallback(inst->userData, inst->id, err_msg);  \
        free(err_msg);                                             \
    }

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
MQTTMessage::MQTTMessage(const Message &message) : Message() {
    if (typeid(message) != typeid(MQTTMessage))
        return;
    MQTTMessage *msg = (MQTTMessage *) &message;
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

MQTTConnection::MQTTConnection(MQTTConnectionParameters &parameters)
    : Connection(parameters) {
    if (mqttInstances == 0)
        libInit();
    mqttInstances++;

    this->mqttParameters = new MQTTConnectionParameters(parameters);
    this->parameters = this->mqttParameters;
    this->mosq = NULL;
    this->queueSize.store(0);
}

MQTTConnection::~MQTTConnection() {
    mqttInstances--;
    if (mqttInstances == 0)
        libCleanup();
    delete this->parameters;
}

void MQTTConnection::setConnectionParameters(ConnectionParameters &parameters) {
    if (typeid(parameters) != typeid(MQTTConnectionParameters))
        return;
    *this->mqttParameters = reinterpret_cast<MQTTConnectionParameters &>(parameters);
    this->parameters = this->mqttParameters;
}

void MQTTConnection::connect() {
    this->status = CONNECTION_STATUS_CONNECTING;
    int ret;

    if (this->mosq == NULL) {
        this->mosq = mosquitto_new(NULL, true, this);
    } else {
        mosquitto_reinitialise(this->mosq, NULL, true, this);
    }

    if (this->mosq == NULL) {
        this->status = CONNECTION_STATUS_ERROR;
        MQTT_ERROR(this, MOSQ_ERR_NOMEM, "Error creating mosquitto instance: ")
        return;
    }

    if (this->mqttParameters->username != "" && this->mqttParameters->password != "") {
        ret = mosquitto_username_pw_set(this->mosq, this->mqttParameters->username.c_str(), this->mqttParameters->password.c_str());
        if (ret != MOSQ_ERR_SUCCESS) {
            this->status = CONNECTION_STATUS_ERROR;
            MQTT_ERROR(this, ret, "Error setting username and password: ")
            return;
        }
    }


    if (this->mqttParameters->tls) {
        ret = mosquitto_tls_set(this->mosq,
            this->mqttParameters->cafile.empty()   ? nullptr : this->mqttParameters->cafile.c_str(),
            this->mqttParameters->capath.empty()   ? nullptr : this->mqttParameters->capath.c_str(),
            this->mqttParameters->certfile.empty() ? nullptr : this->mqttParameters->certfile.c_str(),
            this->mqttParameters->keyfile.empty()  ? nullptr : this->mqttParameters->keyfile.c_str(),
            nullptr);
        if (ret != MOSQ_ERR_SUCCESS) {
            this->status = CONNECTION_STATUS_ERROR;
            MQTT_ERROR(this, ret, "Error setting tls: ")
            return;
        }

        ret = mosquitto_tls_insecure_set(this->mosq, false);
        if (ret != MOSQ_ERR_SUCCESS) {
            this->status = CONNECTION_STATUS_ERROR;
            MQTT_ERROR(this, ret, "Setting TLS: ")
            return;
        }
    }

    ret = mosquitto_loop_start(this->mosq);
    if (ret != MOSQ_ERR_SUCCESS) {
        this->status = CONNECTION_STATUS_ERROR;
        MQTT_ERROR(this, ret, "Error starting mosquitto loop: ")
        return;
    }

    ret = mosquitto_connect_async(this->mosq, this->mqttParameters->host.c_str(), this->mqttParameters->port, 5);
    if (ret != MOSQ_ERR_SUCCESS) {
        this->status = CONNECTION_STATUS_ERROR;
        MQTT_ERROR(this, ret, "Error connecting to broker: ")
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
    mosquitto_loop_stop(this->mosq, true);
    mosquitto_destroy(this->mosq);
    this->mosq = NULL;
    this->status = CONNECTION_STATUS_DISCONNECTED;
}

bool MQTTConnection::send(const Message &message) {
    if (typeid(message) != typeid(MQTTMessage))
        return false;
    if (this->queueSize.load() >= this->maxQueueSize)
        return false;

    MQTTMessage *mqtt_message = (MQTTMessage *) &message;
    mosquitto_publish(
            this->mosq,
            NULL,
            mqtt_message->topic.c_str(),
            mqtt_message->payload.size(),
            mqtt_message->payload.c_str(),
            mqtt_message->qos,
            mqtt_message->retain);
    this->queueSize++;
    return true;
}

void MQTTConnection::receive(Message &message) {
}

bool MQTTConnection::queueSend(const Message &message) {
    return send(message);
}

void MQTTConnection::subscribe(const std::string &topic) {
    mosquitto_subscribe(this->mosq, NULL, topic.c_str(), 0);
}

void MQTTConnection::unsubscribe(const std::string &topic) {
    mosquitto_unsubscribe(this->mosq, NULL, topic.c_str());
}

size_t MQTTConnection::getQueueSize() {
    return this->queueSize;
}

void MQTTConnection::loop() {
}

void MQTTConnection::on_connect(struct mosquitto *mosq, void *obj, int rc) {
    MQTTConnection *connection = (MQTTConnection *) obj;
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
    MQTTConnection *connection = (MQTTConnection *) obj;
    connection->status = CONNECTION_STATUS_DISCONNECTED;
    if (connection->onDisconnectCallback)
        connection->onDisconnectCallback(connection->userData, connection->id);
}

void MQTTConnection::on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    MQTTConnection *connection = (MQTTConnection *) obj;
    MQTTMessage mqtt_message;
    mqtt_message.topic = message->topic;
    mqtt_message.payload = std::string((char *) message->payload, message->payloadlen);
    mqtt_message.timestamp = std::chrono::system_clock::now();
    if (connection->onMessageCallback)
        connection->onMessageCallback(connection->userData, connection->id, mqtt_message);
}

void MQTTConnection::on_publish(struct mosquitto *mosq, void *obj, int mid) {
    MQTTConnection *connection = (MQTTConnection *) obj;
    connection->queueSize--;
}

void MQTTConnection::on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos) {
}

void MQTTConnection::on_unsubscribe(struct mosquitto *mosq, void *obj, int mid) {
}
