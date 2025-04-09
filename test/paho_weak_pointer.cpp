#include "paho_connection_manager.h"

int main() {
  PAHOMQTTConnectionParameters parameters;

  auto conn = std::make_shared<PAHOMQTTConnection>(parameters);

  conn->setOnConnectCallback(
      [](PAHOMQTTConnection *connection, void *userData) {
        std::cout << "[CALLBACK] Connected to MQTT broker" << std::endl;
        connection->subscribe("rec");
        connection->send(PAHOMQTTMessage("test", "Hello from manager!"));
      });
  conn->setOnDisconnectCallback(
      [](PAHOMQTTConnection *connection, void *userData) {
        std::cout << "[CALLBACK] Disconnected from MQTT broker" << std::endl;
      });
  conn->setOnMessageCallback([](PAHOMQTTConnection *connection, void *userData,
                                const PAHOMQTTMessage &message) {
    std::cout << "[CALLBACK] Received: " << message.getTopic() << " -> "
              << message.getPayload() << std::endl;
  });

  PAHOConnectionManager::addConnection(conn);

  {
    auto conn_2 = std::make_shared<PAHOMQTTConnection>(parameters);

    conn_2->setOnConnectCallback(
        [](PAHOMQTTConnection *connection, void *userData) {
          std::cout << "[CALLBACK 2] Connected to MQTT broker" << std::endl;
          connection->subscribe("rec_2");
          connection->send(PAHOMQTTMessage("test", "Hello from manager!"));
        });
    conn_2->setOnDisconnectCallback([](PAHOMQTTConnection *connection,
                                       void *userData) {
      std::cout << "[CALLBACK 2] Disconnected from MQTT broker" << std::endl;
    });
    conn_2->setOnMessageCallback([](PAHOMQTTConnection *connection,
                                    void *userData,
                                    const PAHOMQTTMessage &message) {
      std::cout << "[CALLBACK 2] Received: " << message.getTopic() << " -> "
                << message.getPayload() << std::endl;
    });

    PAHOConnectionManager::addConnection(conn_2);

    PAHOConnectionManager::start();

    std::cout << "Press enter to destroy conn_2" << std::endl;
    std::cin.get();
  }

  std::cout << "conn_2 is now out of scope" << std::endl;
  std::cout << "Press enter to quit ..." << std::endl;
  std::cin.get();

  PAHOConnectionManager::disconnect_all();
  PAHOConnectionManager::stop();

  return 0;
}
