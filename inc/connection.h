// Abstact class to know which function to implement for different connection types

#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <queue>
#include <mutex>
#include <thread>
#include <string>
#include <unistd.h>
#include <functional>
#include <condition_variable>

using namespace std;

enum ConnectionState_
{
	NONE,
	CONNECTING,
	CONNECTED,
	FAIL,
	CLOSED
};

class GenericSocket
{
};
class GenericMessage
{
public:
    GenericMessage(){
        topic = "";
        payload = "";
    };
    GenericMessage(const string &topic_, const string &payload_) : topic(topic_), payload(payload_){};

    // use these if in subsub
    string topic;
    string payload;
};

class Connection
{
public:
    ~Connection();
    int getId();

    void init(const string &address, const string &port, const int &openMode);

    enum
    {
        NONE,
        PUB,
        SUB
    };

    struct message
    {
        string topic;
        string payload;
    };

    virtual void closeConnection() = 0;
    virtual thread *start() = 0;

    virtual int subscribe(const string &topic) = 0;
    virtual int unsubscribe(const string &topic) = 0;

    void clearData();
    void setData(const GenericMessage &message);
    inline int getQueueSize(){return max_queue_size - buff_send.size();};

    void addOnConnect(function<void(const int &id)>);
    void addOnDisconnect(function<void(const int &id, const int &code)>);

    void addOnError(function<void(const int &id, const int &code, const string &msg)>);

    void addOnMessage(function<void(const int &id, const GenericMessage &msg)>);
    void addOnPublish(function<void(const int &id, const string &topic)>);

    void addOnSubscribe(function<void(const int &id, const string &topic)>);
    void addOnUnsubscribe(function<void(const int &id, const string &topic)>);

    // ZMQ/RAW_TCP/WEBSOCKET...
    string GetConnectionType() { return connection_type; };

protected:
    Connection();

    int id;
    int max_queue_size;
    int subscription_count;

    string port;
    string address;
    string connection_type;
    int openMode;

    bool open = false;
    bool done = false;
    bool new_data = false;

    mutex mtx;
    condition_variable cv;
    queue<GenericMessage> buff_send;

protected:
    void stop();
    void reset();

    void sendLoop();
    void receiveLoop();

    virtual void sendMessage(const GenericMessage &msg) = 0;
    virtual void receiveMessage(GenericMessage &msg) = 0;

    function<void(const int &id)> onConnect;
    function<void(const int &id, const int &code)> onDisconnect;

    function<void(const int &id, const int &code, const string &msg)> onError;

    function<void(const int &id, const GenericMessage &msg)> onMessage;
    function<void(const int &id, const string &topic)> onPublish;

    function<void(const int &id, const string &topic)> onSubscribe;
    function<void(const int &id, const string &topic)> onUnsubscribe;

private:
    static int instance_count;
};

#endif