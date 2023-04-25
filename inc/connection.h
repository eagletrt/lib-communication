#pragma once

#include <mutex>
#include <queue>
#include <condition_variable>

class Message{
public:
  virtual ~Message(){};
};
class ConnectionParameters{
public:
  virtual ~ConnectionParameters(){};
};

enum ConnectionStatus{
  CONNECTED = 0,
  CONNECTING,
  DISCONNECTED,
  ERROR,
  CONNECTION_STATUS_COUNT
};

typedef void (*OnConnectCallback)(void* userData, int id);
typedef void (*OnDisconnectCallback)(void* userData, int id);
typedef void (*OnMessageCallback)(void* userData, int id, const Message& message);
typedef void (*OnErrorCallback)(void* userData, int id, const char* error);

class Connection{
public:
    Connection(ConnectionParameters& parameters);
    virtual ~Connection();

    virtual void setConnectionParameters(ConnectionParameters& parameters) = 0;
    void setUserData(void* userData); // used for callbacks

    virtual void connect() = 0;
    virtual void disconnect() = 0;

    virtual void send(const Message& message) = 0;
    virtual void receive(Message& message) = 0;
    virtual void queueSend(const Message& message) = 0;

    void setOnConnectCallback(OnConnectCallback callback);
    void setOnDisconnectCallback(OnDisconnectCallback callback);
    void setOnMessageCallback(OnMessageCallback callback);
    void setOnErrorCallback(OnErrorCallback callback);

    ConnectionStatus getStatus() const;
    
protected:
    static int connectionCount;
    int id;
    void* userData;

    ConnectionParameters* parameters = NULL;
    ConnectionStatus status;

    OnConnectCallback onConnectCallback;
    OnDisconnectCallback onDisconnectCallback;
    OnMessageCallback onMessageCallback;
    OnErrorCallback onErrorCallback;

    std::mutex messageQueueMutex;
    std::condition_variable messageQueueCondition;
    std::queue<Message *> messageQueue;

    virtual void loop() = 0;
};