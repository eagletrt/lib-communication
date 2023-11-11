#include "connection_manager.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>

namespace ConnectionManager{
  std::vector<Connection*> connections;
  std::thread* connectionThread = NULL;
  std::mutex connectionMutex;
  std::condition_variable connectionCondition;
  std::atomic<bool> connectionThreadRunning = false;

  static void connectionThreadFunction(){
    connectionThreadRunning = true;
    while(connectionThreadRunning.load()){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      for(auto connection : connections){
        if(connection->getStatus() == CONNECTION_STATUS_CONNECTED || connection->getStatus() == CONNECTION_STATUS_CONNECTING)
          continue;
        connection->connect();
      }
    }
  }

  void start(){
    if(connectionThreadRunning)
      return;
    connectionThread = new std::thread(connectionThreadFunction);
  }

  void stop(){
    if(!connectionThreadRunning)
      return;
    connectionThreadRunning = false;
    if(connectionThread != NULL && connectionThread->joinable())
      connectionThread->join();
  }

  bool addConnection(Connection* connection){
    for(const auto* conn: connections)
      if(conn == connection)
        return false;
    connections.push_back(connection);
    return true;
  }

  bool removeConnection(Connection* connection){
    for(int i = 0; i < connections.size(); i++){
      if(connections[i] == connection){
        connections.erase(connections.begin() + i);
        return true;
      }
    }
    return false;
  }

  void connect_all(){
    for(auto connection : connections)
      connection->connect();
  }

  void disconnect_all(){
    for(auto connection : connections)
      connection->disconnect();
  }
}