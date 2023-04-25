#pragma once

#include "connection.h"

namespace ConnectionManager
{
  void start();
  void stop();

  bool addConnection(Connection* connection);
  bool removeConnection(Connection* connection);

  void connect_all();
  void disconnect_all();
};