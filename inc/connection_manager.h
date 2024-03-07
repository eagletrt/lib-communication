#pragma once

#include "connection.h"
#include <atomic>
#include <mutex>

template <class UserDataType>
class ConnectionManager {
public:
  void start();
  void stop();
  bool addConnection(Connection<UserDataType>* connection);
  bool removeConnection(Connection<UserDataType>* connection);
  void connect_all();
  void disconnect_all();

private:
  std::thread *connection_thread;
  std::atomic<bool> is_thread_running;
  std::mutex connection_mutex;
  std::vector<Connection<UserDataType>*> connections;

  void connectionThreadFunction();
};
