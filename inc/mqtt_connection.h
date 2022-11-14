#ifndef __MQTT_CONNECTION__
#define __MQTT_CONNECTION__

#include <set>
#include <thread>

#include "mosquitto.h"

#include "connection.h"

using namespace std;

#define MOSQ_AUTH_FAIL 5

class custom_mqtt_socket : public GenericSocket {
    public:
        mosquitto* client;

};

class MQTTConnection;

struct MQTTData {
    MQTTConnection* inst;
    function<void(struct mosquitto* client, void* data, int result)> clbk_on_connect;
    function<void(struct mosquitto* client, void* data, int result)> clbk_on_disconnect;

    function<void(struct mosquitto* client, void* data, const struct mosquitto_message* msg)> clbk_on_message;
    function<void(struct mosquitto* client, void* data, int mid)> clbk_on_publish;

    function<void(struct mosquitto* client, void* data, int mid, int qos_count, const int* granted_qos)> clbk_on_subscribe;
    function<void(struct mosquitto* client, void* data, int mid)> clbk_on_unsubscribe;

    function<void(const int &id, const int &code, const string &msg)> clbk_on_error;

};

class MQTTConnection : public Connection {
    public:
        MQTTConnection();
        MQTTConnection(const string &username, const string &password);
        ~MQTTConnection();

        virtual void closeConnection();
        virtual thread *start();

        virtual int subscribe(const string &topic);
        virtual int unsubscribe(const string &topic);

        int getPendingMessages();

        void reconnect();

    private:
        int pending_messages;
        MQTTData mqtt_data;
        custom_mqtt_socket *socket;
        string username = "", password = "";

        thread *telemetry_thread = nullptr;

        void m_on_connect(struct mosquitto* client, void* data, int result);
        void m_on_publish(struct mosquitto* client, void* data, int mid);
        void m_on_disconnect(struct mosquitto* client, void* data, int result);
        void m_on_message(struct mosquitto* client, void* data, const struct mosquitto_message* msg);
        void m_on_error(const int &id, const int &code, const string &msg);
        void m_on_subscribe(struct mosquitto* client, void* data, int mid, int qos_count, const int* granted_qos);
        void m_on_unsubscribe(struct mosquitto* client, void* data, int mid);

        bool error_check(const int &res, const string &msg);

        void authenticate();

        virtual void sendMessage(const GenericMessage &msg);
        virtual void receiveMessage(GenericMessage &msg);
};

#endif