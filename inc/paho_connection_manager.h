#pragma once

#include "paho_mqtt_connection.hpp"

namespace PAHOConnectionManager {
void start();
void stop();

bool addConnection(std::shared_ptr<PAHOMQTTConnection> connection);
bool removeConnection(std::shared_ptr<PAHOMQTTConnection> connection);

void connect_all();
void disconnect_all();
void disconnect_all_and_idle();
};  // namespace PAHOConnectionManager
