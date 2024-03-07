#include "connection.h"
#include <memory>
#include <mutex>

template <class UserDataType>
int Connection<UserDataType>::connectionCount = 0;

template <class UserDataType>
Connection<UserDataType>::Connection() : Connection(ConnectionParameters()) {}

template <class UserDataType>
Connection<UserDataType>::Connection(const ConnectionParameters &parameters) {
	this->status = CONNECTION_STATUS_DISCONNECTED;
	this->id = connectionCount++;
	this->userData = NULL;
	this->maxQueueSize = 1500;
	this->parameters = parameters;
}

template <class UserDataType>
Connection<UserDataType>::Connection(Connection &&other)
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

template <class UserDataType>
Connection<UserDataType>::~Connection() {
	while (messageQueue.size() > 0) {
		delete messageQueue.front();
		messageQueue.pop();
	}
}

template <class UserDataType>
size_t Connection<UserDataType>::getMaxQueueSize() { return this->maxQueueSize; }

template <class UserDataType>
void Connection<UserDataType>::setMaxQueueSize(size_t size) { this->maxQueueSize = size; }

template <class UserDataType>
void Connection<UserDataType>::setUserData(UserDataType userData) {
  this->userData = std::make_unique(userData);
}

template <class UserDataType>
void Connection<UserDataType>::setOnConnectCallback(OnConnectCallback callback) { this->onConnectCallback = callback; }

template <class UserDataType>
void Connection<UserDataType>::setOnDisconnectCallback(OnDisconnectCallback callback) { this->onDisconnectCallback = callback; }

template <class UserDataType>
void Connection<UserDataType>::setOnMessageCallback(OnMessageCallback callback) { this->onMessageCallback = callback; }

template <class UserDataType>
void Connection<UserDataType>::setOnErrorCallback(OnErrorCallback callback) { this->onErrorCallback = callback; }

template <class UserDataType>
ConnectionStatus Connection<UserDataType>::getStatus() const { return this->status; }
