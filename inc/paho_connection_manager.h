#pragma once

#include "paho_mqtt_connection.hpp"

namespace PAHOConnectionManager {
void start();
void stop();

bool addConnection(PAHOMQTTConnection *connection);
bool removeConnection(PAHOMQTTConnection *connection);

void connect_all();
void disconnect_all();
}; // namespace PAHOConnectionManager
