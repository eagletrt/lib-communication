#include "connection.h"

int Connection::connectionCount = 0;

Connection::Connection(ConnectionParameters& parameters) {
    this->status = DISCONNECTED;
    this->id = connectionCount++;
    this->userData = NULL;
}

Connection::~Connection() {
  while(messageQueue.size() > 0) {
    delete messageQueue.front();
    messageQueue.pop();
  }
}

void Connection::setUserData(void* userData) {
    this->userData = userData;
}

void Connection::setOnConnectCallback(OnConnectCallback callback) {
    this->onConnectCallback = callback;
}

void Connection::setOnDisconnectCallback(OnDisconnectCallback callback) {
    this->onDisconnectCallback = callback;
}

void Connection::setOnMessageCallback(OnMessageCallback callback) {
    this->onMessageCallback = callback;
}

void Connection::setOnErrorCallback(OnErrorCallback callback) {
    this->onErrorCallback = callback;
}

ConnectionStatus Connection::getStatus() const {
    return this->status;
}