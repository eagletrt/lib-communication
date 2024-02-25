#pragma once

#include <mutex>
#include <queue>
#include <condition_variable>

class Message {
public:
	virtual ~Message(){};
};
class ConnectionParameters {
public:
	virtual ~ConnectionParameters(){};
};

enum ConnectionStatus {
	CONNECTION_STATUS_CONNECTED = 0,
	CONNECTION_STATUS_CONNECTING,
	CONNECTION_STATUS_DISCONNECTED,
	CONNECTION_STATUS_ERROR,
	CONNECTION_STATUS_COUNT
};

typedef void (*OnConnectCallback)(void *userData, int id);
typedef void (*OnDisconnectCallback)(void *userData, int id);
typedef void (*OnMessageCallback)(void *userData, int id, const Message &message);
typedef void (*OnErrorCallback)(void *userData, int id, const char *error);

class Connection {
public:
	Connection();
	Connection(const ConnectionParameters &parameters);
	Connection(Connection &&);
	virtual ~Connection();

	void setMaxQueueSize(size_t size);
	size_t getMaxQueueSize();
	int getInstanceID() { return id; };

	virtual void setConnectionParameters(const ConnectionParameters &parameters) = 0;
	const ConnectionParameters &getConnectionParameters() const { return parameters; };
	void setUserData(void *userData); // used for callbacks

	virtual void connect() = 0;
	virtual void disconnect() = 0;

	virtual bool send(const Message &message) = 0;
	virtual void receive(Message &message) = 0;
	virtual bool queueSend(const Message &message) = 0;

	void setOnConnectCallback(OnConnectCallback callback);
	void setOnDisconnectCallback(OnDisconnectCallback callback);
	void setOnMessageCallback(OnMessageCallback callback);
	void setOnErrorCallback(OnErrorCallback callback);

	ConnectionStatus getStatus() const;
	virtual size_t getQueueSize() = 0;

protected:
	static int connectionCount;
	int id;
	void *userData;
	size_t maxQueueSize;

	ConnectionParameters parameters;
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
