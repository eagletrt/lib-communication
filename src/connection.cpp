#include "connection.h"
#include <mutex>

int Connection::connectionCount = 0;

Connection::Connection() : Connection(ConnectionParameters()) {}
Connection::Connection(const ConnectionParameters &parameters) {
	this->status = CONNECTION_STATUS_DISCONNECTED;
	this->id = connectionCount++;
	this->userData = NULL;
	this->maxQueueSize = 1500;
	this->parameters = parameters;
}
Connection::Connection(Connection &&other)
		: id(other.id), userData(other.userData), maxQueueSize(other.maxQueueSize), parameters(std::move(other.parameters)),
			status(other.status), onConnectCallback(other.onConnectCallback),
			onDisconnectCallback(other.onDisconnectCallback), onMessageCallback(other.onMessageCallback),
			onErrorCallback(other.onErrorCallback), messageQueueMutex(), messageQueueCondition(),
			messageQueue(std::move(other.messageQueue)) {
	// Reset the other object's data
	other.id = -1;
	other.userData = nullptr;
	other.maxQueueSize = 0;
	other.status = CONNECTION_STATUS_DISCONNECTED;
	other.onConnectCallback = nullptr;
	other.onDisconnectCallback = nullptr;
	other.onMessageCallback = nullptr;
	other.onErrorCallback = nullptr;
}

Connection::~Connection() {
	while (messageQueue.size() > 0) {
		delete messageQueue.front();
		messageQueue.pop();
	}
}

size_t Connection::getMaxQueueSize() { return this->maxQueueSize; }

void Connection::setMaxQueueSize(size_t size) { this->maxQueueSize = size; }

void Connection::setUserData(void *userData) { this->userData = userData; }

void Connection::setOnConnectCallback(OnConnectCallback callback) { this->onConnectCallback = callback; }

void Connection::setOnDisconnectCallback(OnDisconnectCallback callback) { this->onDisconnectCallback = callback; }

void Connection::setOnMessageCallback(OnMessageCallback callback) { this->onMessageCallback = callback; }

void Connection::setOnErrorCallback(OnErrorCallback callback) { this->onErrorCallback = callback; }

ConnectionStatus Connection::getStatus() const { return this->status; }
