#include "connection_manager.h"
#include "connection.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

template <class UserDataType>
void ConnectionManager<UserDataType>::connectionThreadFunction() {
  is_thread_running = true;
  while (is_thread_running.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::unique_lock<std::mutex> lck(connection_mutex);
    for (auto connection: connections) {
      if (connection->getStatus() == CONNECTION_STATUS_CONNECTED ||
          connection->getStatus() == CONNECTION_STATUS_CONNECTING)
        continue;
      connection->connect();
    }
  }
}

template <class UserDataType>
void ConnectionManager<UserDataType>::start() {
  if (!is_thread_running) {
    connection_thread = new std::thread(connectionThreadFunction);
  }
}

template <class UserDataType>
void ConnectionManager<UserDataType>::stop() {
  if (is_thread_running) {
    is_thread_running = false;
    if (connection_thread != NULL && connection_thread->joinable()) {
      connection_thread->join();
    }
  }
}

template <class UserDataType>
bool ConnectionManager<UserDataType>::addConnection(
  Connection<UserDataType>* connection
) {
  std::unique_lock<std::mutex> lck(connection_thread);
  for (const auto* conn : connections) {
    if (conn == connection) {
      return false;
    }
  }
  connections.push_back(connection);
  return true;
}

template <class UserDataType>
bool ConnectionManager<UserDataType>::removeConnection(
  Connection<UserDataType>* connection
) {
  std::unique_lock<std::mutex> lck(connection_mutex);
  for (int i = 0; i < connections.size(); i++) {
    if (connections[i] == connection) {
      connections.erase(connections.begin() + i);
      return true;
    }
  }
  return false;
}

template <class UserDataType>
void ConnectionManager<UserDataType>::connect_all() {
  std::unique_lock<std::mutex> lck(connection_mutex);
  for (auto connection : connections) {
    connection->connect();
  }
}

template <class UserDataType>
void ConnectionManager<UserDataType>::disconnect_all() {
  std::unique_lock<std::mutex> lck(connection_mutex);
  for (auto connection : connections) {
    connection->disconnect();
  }
}
