#include "paho_mqtt_connection.hpp"

int main(void) {
  PAHOMQTTConnectionParameters parameters;
  PAHOMQTTConnection connection(parameters);
  connection.connect();

  while (connection.getStatus() != PAHOMQTTConnectionStatus::CONNECTED) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  connection.setWillMessage(PAHOMQTTMessage("test", "last will"));

  connection.send(PAHOMQTTMessage("test", "test"));
  connection.subscribe("rec");

  connection.setOnConnectCallback(
      [](PAHOMQTTConnection *connection, void *userData) {
        std::cout << "Connected" << std::endl;
      });
  connection.setOnDisconnectCallback(
      [](PAHOMQTTConnection *connection, void *userData) {
        std::cout << "Disconnected" << std::endl;
      });
  connection.setOnMessageCallback([](PAHOMQTTConnection *connection,
                                     void *userData,
                                     const PAHOMQTTMessage &message) {
    std::cout << "Message received: " << message.getTopic() << " "
              << message.getPayload() << std::endl;
  });

  std::cin.get();

  connection.disconnect();

  std::cin.get();
  return 0;
}