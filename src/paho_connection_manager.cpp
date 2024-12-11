#include "paho_connection_manager.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <paho_mqtt_connection.hpp>
#include <thread>
#include <vector>

namespace PAHOConnectionManager {
std::vector<PAHOMQTTConnection *> connections;
std::unique_ptr<std::thread> connectionThread = NULL;
std::mutex connectionMutex;
std::condition_variable connectionCondition;
std::atomic<bool> connectionThreadRunning = false;

static void connectionThreadFunction() {
  connectionThreadRunning = true;
  while (connectionThreadRunning.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::unique_lock<std::mutex> lck(connectionMutex);
    for (auto connection : connections) {
      if (connection->getStatus() == PAHOMQTTConnectionStatus::CONNECTED ||
          connection->getStatus() == PAHOMQTTConnectionStatus::CONNECTING)
        continue;
      connection->connect();
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

bool addConnection(PAHOMQTTConnection *connection) {
  std::unique_lock<std::mutex> lck(connectionMutex);
  for (const auto *conn : connections)
    if (conn == connection)
      return false;
  connections.push_back(connection);
  return true;
}

bool removeConnection(PAHOMQTTConnection *connection) {
  std::unique_lock<std::mutex> lck(connectionMutex);
  for (int i = 0; i < connections.size(); i++) {
    if (connections[i] == connection) {
      connections.erase(connections.begin() + i);
      return true;
    }
  }
  return false;
}

void connect_all() {
  std::unique_lock<std::mutex> lck(connectionMutex);
  for (auto connection : connections)
    connection->connect();
}

void disconnect_all() {
  std::unique_lock<std::mutex> lck(connectionMutex);
  for (auto connection : connections)
    connection->disconnect();
}
} // namespace PAHOConnectionManager
