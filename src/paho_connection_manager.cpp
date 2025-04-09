#include "paho_connection_manager.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <paho_mqtt_connection.hpp>
#include <thread>
#include <vector>

namespace PAHOConnectionManager {
std::vector<std::weak_ptr<PAHOMQTTConnection>> connections;
std::unique_ptr<std::thread> connectionThread = NULL;
std::mutex connectionMutex;
std::condition_variable connectionCondition;
std::atomic<bool> connectionThreadRunning = false;

static void connectionThreadFunction() {
  connectionThreadRunning = true;
  while (connectionThreadRunning.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::unique_lock<std::mutex> lck(connectionMutex);
    auto conns = connections;
    connections.clear();

    for (auto weak_conn : conns) {
      if (auto connection = weak_conn.lock()) {
        if (connection->getStatus() == PAHOMQTTConnectionStatus::CONNECTED ||
            connection->getStatus() == PAHOMQTTConnectionStatus::CONNECTING) {
          connections.push_back(connection);
          continue;
        }
        connection->connect();
        connections.push_back(connection);
      }
    }
  }
}

void start() {
  if (connectionThreadRunning)
    return;
  connectionThread = std::make_unique<std::thread>(connectionThreadFunction);
}

void stop() {
  if (!connectionThreadRunning)
    return;
  connectionThreadRunning = false;
  if (connectionThread != NULL && connectionThread->joinable())
    connectionThread->join();
}

bool addConnection(std::shared_ptr<PAHOMQTTConnection> connection) {
  std::unique_lock<std::mutex> lck(connectionMutex);
  for (const auto &weak_conn : connections) {
    if (auto conn = weak_conn.lock()) {
      if (conn == connection) {
        return false;
      }
    }
  }
  connections.push_back(connection);
  return true;
}

bool removeConnection(std::shared_ptr<PAHOMQTTConnection> connection) {
  std::unique_lock<std::mutex> lck(connectionMutex);
  for (auto it = connections.begin(); it != connections.end(); ++it) {
    if (auto conn = it->lock()) {
      if (conn == connection) {
        connections.erase(it);
        return true;
      }
    }
  }
  return false;
}

void connect_all() {
  std::unique_lock<std::mutex> lck(connectionMutex);
  for (auto weak_conn : connections) {
    if (auto connection = weak_conn.lock()) {
      connection->connect();
    }
  }
}

void disconnect_all() {
  std::unique_lock<std::mutex> lck(connectionMutex);
  for (auto weak_conn : connections) {
    if (auto connection = weak_conn.lock()) {
      connection->disconnect();
    }
  }
}
} // namespace PAHOConnectionManager
