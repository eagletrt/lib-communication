#include "mqtt_connection.h"

#include <sstream>
#include <iostream>

using namespace std;

void on_message(struct mosquitto* client, void* data, const struct mosquitto_message* msg) {
    MQTTData *mqtt_data = (MQTTData*) data;

    mqtt_data->clbk_on_message(client, data, msg);
}

void on_connect(struct mosquitto* client, void* data, int result) {
    MQTTData *mqtt_data = (MQTTData*) data;

    mqtt_data->clbk_on_connect(client, data, result);
}

void on_disconnect(struct mosquitto* client, void* data, int result) {
    MQTTData *mqtt_data = (MQTTData*) data;

    mqtt_data->clbk_on_disconnect(client, data, result);
}

void on_publish(struct mosquitto* client, void* data, int mid) {
    MQTTData *mqtt_data = (MQTTData*) data;

    mqtt_data->clbk_on_publish(client, data, mid);
}

void on_subscribe(struct mosquitto* client, void* data, int mid, int qos_count, const int* granted_qos) {
    MQTTData *mqtt_data = (MQTTData*) data;

    mqtt_data->clbk_on_subscribe(client, mqtt_data, mid, qos_count, granted_qos);
}

void on_unsubscribe(struct mosquitto* client, void* data, int mid) {
    MQTTData *mqtt_data = (MQTTData*) data;

    mqtt_data->clbk_on_unsubscribe(client, mqtt_data, mid);
}

MQTTConnection::MQTTConnection() : Connection() {
    socket = new custom_mqtt_socket();

    int res;
    string err_msg;

    socket->client = mosquitto_new(NULL, true, (void*)&mqtt_data);

    if(!socket->client) {
        switch(errno) {
            case ENOMEM:
                err_msg = "Error while initializing: Out of memory";
                break;
            case EINVAL:
                err_msg = "Error while initializing: Invalid input parameters";
                break;
            default:
                err_msg = "Error while initializing: Unknown error";
                break;
        }
        if(error_check(errno, err_msg)) {
            return;
        }
    }

    mqtt_data.inst = this;

    mqtt_data.clbk_on_connect = std::bind(&MQTTConnection::m_on_connect, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    mqtt_data.clbk_on_disconnect = std::bind(&MQTTConnection::m_on_disconnect, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    mqtt_data.clbk_on_error = std::bind(&MQTTConnection::m_on_error, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    mosquitto_connect_callback_set(socket->client, on_connect);
    mosquitto_disconnect_callback_set(socket->client, on_disconnect);

    connection_type = "MQTT";
}

MQTTConnection::MQTTConnection(const string &username, const string &password) : MQTTConnection() {
    this->username = username;
    this->password = password;
}

MQTTConnection::~MQTTConnection() {
    mosquitto_destroy(socket->client);

    delete socket;

    mosquitto_lib_cleanup();
}

thread* MQTTConnection::start() {
    int res = MOSQ_ERR_INVAL, keepalive = 10, port_int = 1883;
    string err_msg;

    done = false;

    authenticate();

    if(openMode == SUB) {
        mqtt_data.clbk_on_message = std::bind(&MQTTConnection::m_on_message, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        mosquitto_message_callback_set(socket->client, on_message);

        mqtt_data.clbk_on_subscribe = std::bind(&MQTTConnection::m_on_subscribe, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
        mosquitto_subscribe_callback_set(socket->client, on_subscribe);

        mqtt_data.clbk_on_unsubscribe = std::bind(&MQTTConnection::m_on_unsubscribe, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        mosquitto_unsubscribe_callback_set(socket->client, on_unsubscribe);
    } else if(openMode == PUB) {
        mqtt_data.clbk_on_publish = std::bind(&MQTTConnection::m_on_publish, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        mosquitto_publish_callback_set(socket->client, on_publish);
    }

    port_int = strtol(port.c_str(), NULL, 10);
    if((port_int == 0 && errno != 0) || port_int > 65535) {
        if(onError)
            onError(id, 0, "Invalid port number");
        return nullptr;
    }
    if(address.size() <= 4) {
        if(onError)
            onError(id, 0, "Missing socket address");
        return nullptr;
    }
    
    res = mosquitto_connect(socket->client, address.c_str(), port_int, keepalive);
    switch(res) {
        case MOSQ_ERR_INVAL:
            err_msg = "Error while connecting: Invalid input parameters";
            break;
        case MOSQ_ERR_ERRNO:
            err_msg = "Error while connecting: System error";
            break;
        case MOSQ_ERR_SUCCESS:
            break;
        default:
            err_msg = "Error while connecting: Unknown error";
            break;
    }

    if(error_check(res, err_msg)) {
        return nullptr;
    }

    res = mosquitto_loop_start(socket->client);
    switch(res) {
        case MOSQ_ERR_INVAL:
            err_msg = "Error while starting the loop: Invalid input parameters";
            break;
        case MOSQ_ERR_SUCCESS:
            break;
        case MOSQ_ERR_NOT_SUPPORTED:
            err_msg = "Error while starting the loop: Thread support not available";
            break;
        default:
            err_msg = "Error while starting the loop: Unknown error";
            break;
    }

    if(error_check(res, err_msg)) {
        return nullptr;
    }

    if(openMode == PUB)
        telemetry_thread = new thread(&MQTTConnection::sendLoop, this);
    else if(openMode == SUB)
        telemetry_thread = new thread(&MQTTConnection::receiveLoop, this);

    return telemetry_thread;
}

void MQTTConnection::reconnect() {
    int res = mosquitto_reconnect(socket->client);
    string err_msg;

    switch(res) {
        case MOSQ_ERR_INVAL:
            err_msg = "Error while reconnecting: Invalid input parameters";
            break;
        case MOSQ_ERR_NOMEM:
            err_msg = "Error while reconnecting: Out of memory";
            break;
        case MOSQ_ERR_ERRNO:
            err_msg = "Error while reconnecting: System error";
            break;
        case MOSQ_ERR_SUCCESS:
            break;
        default:
            err_msg = "Error while reconnecting: Unknown error";
            break;
    }

    if(error_check(res, err_msg)) {
        return;
    }
}

void MQTTConnection::authenticate() {
    if(username != "" && password != "") {
        int res = mosquitto_username_pw_set(socket->client, username.c_str(), password.c_str());
        string err_msg;

        switch(res) {
            case MOSQ_ERR_INVAL:
                err_msg = "Error while authenticating: Invalid input parameters";
                break;
            case MOSQ_ERR_NOMEM:
                err_msg = "Error while authenticating: Out of memory";
                break;
            case MOSQ_ERR_SUCCESS:
                break;
            default:
                err_msg = "Error while authenticating: Unknown error";
                break;
        }

        if(error_check(res, err_msg)) {
            return;
        }
    }
}

void MQTTConnection::closeConnection() {
    if(open) {
        int res = mosquitto_disconnect(socket->client);
        mosquitto_loop_stop(socket->client, false);
        string err_msg;

        switch(res) {
            case MOSQ_ERR_INVAL:
                err_msg = "Error while disconnecting: Invalid input parameters";
                break;
            case MOSQ_ERR_NO_CONN:
                err_msg = "Error while disconnecting: Not connected";
                break;
            case MOSQ_ERR_SUCCESS:
                break;
            default:
                err_msg = "Error while disconnecting: Unknown error";
                break;
        }

        if(error_check(res, err_msg)) {
            return;
        }
    }
    stop();
}

int MQTTConnection::subscribe(const string &topic) {
    int res = mosquitto_subscribe(socket->client, NULL, topic.c_str(), 0);

    string err_msg;

    switch(res) {
        case MOSQ_ERR_INVAL:
            err_msg = "Error while subscribing: Invalid input parameters";
            break;
        case MOSQ_ERR_NO_CONN:
            err_msg = "Error while subscribing: Not connected";
            break;
        case MOSQ_ERR_NOMEM:
            err_msg = "Error while subscribing: Out of memory";
            break;
        case MOSQ_ERR_SUCCESS:
            break;
        default:
            err_msg = "Error while subscribing: Unknown error";
            break;
    }

    if(error_check(res, err_msg)) {
        return res;
    } else {
        subscription_count++;
    }

    return res;
}

int MQTTConnection::unsubscribe(const string &topic) {
    int res = mosquitto_unsubscribe(socket->client, NULL, topic.c_str());
    string err_msg;

    switch(res) {
        case MOSQ_ERR_INVAL:
            err_msg = "Error while unsubscribing: Invalid input parameters";
            break;
        case MOSQ_ERR_NO_CONN:
            err_msg = "Error while unsubscribing: Not connected";
            break;
        case MOSQ_ERR_NOMEM:
            err_msg = "Error while unsubscribing: Out of memory";
            break;
        case MOSQ_ERR_SUCCESS:
            break;
        default:
            err_msg = "Error while unsubscribing: Unknown error";
            break;
    }

    if(error_check(res, err_msg)) {
        return res;
    } else {
        subscription_count--;
    }

    return res;
}

int MQTTConnection::getPendingMessages() {
    return pending_messages;
}

void MQTTConnection::sendMessage(const GenericMessage &msg) {
    int res = mosquitto_publish(socket->client, NULL, msg.topic.c_str(), msg.payload.size(), msg.payload.c_str(), 0, false);
    pending_messages++;
    string err_msg;

    switch(res) {
        case MOSQ_ERR_INVAL:
            err_msg = "Error while publishing: Invalid input parameters";
            break;
        case MOSQ_ERR_NO_CONN:
            err_msg = "Error while publishing: Not connected";
            break;
        case MOSQ_ERR_NOMEM:
            err_msg = "Error while publishing: Out of memory";
            break;
        case MOSQ_ERR_SUCCESS:
            break;
        default:
            err_msg = "Error while publishing: Unknown error";
            break;
    }

    if(error_check(res, err_msg)) {
        return;
    }
}

void MQTTConnection::receiveMessage(GenericMessage &msg) {
    unique_lock<mutex> lck(mtx);
    cv.wait(lck);
}

bool MQTTConnection::error_check(const int &res, const string &err_msg) {
    if(res != MOSQ_ERR_SUCCESS) {
        cout << err_msg << endl;
        // mosquitto_destroy(socket->client);
        if(mqtt_data.clbk_on_error)
            mqtt_data.clbk_on_error(id, res, err_msg);
        return true;
    }
    return false;
}

void MQTTConnection::m_on_connect(struct mosquitto* client, void* data, int result) {
    open = true;

    if(result == MOSQ_AUTH_FAIL) {
        if(onError)
            onError(id, result, "Authentication failed");
    } else {
        if(onConnect)
            onConnect(getId());
    }
}

void MQTTConnection::m_on_disconnect(struct mosquitto* client, void* data, int result) {
    if(onDisconnect){
        onDisconnect(getId(), result);
    }
}

void MQTTConnection::m_on_message(struct mosquitto* client, void* data, const struct mosquitto_message* msg) {
    if(onMessage) {
        onMessage(getId(), GenericMessage(msg->topic, (char*) msg->payload));
    }
}

void MQTTConnection::m_on_publish(struct mosquitto* client, void* data, int mid) {
    pending_messages--;
    if(onPublish) {
        onPublish(getId(), "Unknown topic");
    }
}

void MQTTConnection::m_on_error(const int &id, const int &code, const string &msg) {
    if(onError)
        onError(id, code, msg);
}

void MQTTConnection::m_on_subscribe(struct mosquitto* client, void* data, int mid, int qos_count, const int* granted_qos) {
    if(onSubscribe)
        onSubscribe(id, "Unknown topic");
}

void MQTTConnection::m_on_unsubscribe(struct mosquitto* client, void* data, int mid) {
    if(onUnsubscribe)
        onUnsubscribe(id, "Unknown topic");
}