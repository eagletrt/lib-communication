#include "paho_mqtt_connection.hpp"

#include <assert.h>

#include <functional>
#include <sstream>

#include "mqtt/async_client.h"
#include "mqtt/token.h"

int PAHOMQTTConnection::instanceCounter = 0;

std::string generateID(int instanceCounter) {
  auto now = std::chrono::system_clock::now();
  auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  auto epoch = now_ms.time_since_epoch();
  long long timestamp = epoch.count();

  std::stringstream ss;
  ss << timestamp << instanceCounter;
  std::string input = ss.str();

  unsigned long hash = 5381;
  for (char c : input) {
    hash = ((hash << 5) + hash) + c;  // hash * 33 + c
  }

  std::stringstream hashString;
  hashString << std::hex << hash;

  return hashString.str().substr(0, 30);
}

PAHOMQTTMessage::PAHOMQTTMessage() : PAHOMQTTMessage("", "", 0, false) {};
PAHOMQTTMessage::PAHOMQTTMessage(const std::string &topic,
                                 const std::string &payload)
    : PAHOMQTTMessage(topic, payload, 0, false) {};
PAHOMQTTMessage::PAHOMQTTMessage(const std::string &topic,
                                 const std::string &payload, int qos,
                                 bool retain)
    : qos(qos), retain(retain), topic(topic), payload(payload) {};

PAHOMQTTConnectionParameters::PAHOMQTTConnectionParameters()
    : uri("mqtt://localhost:1883"),
      username(""),
      password(""),
      tls(false),
      cafile(""),
      capath(""),
      certfile(""),
      keyfile("") {};

PAHOMQTTConnectionParameters::~PAHOMQTTConnectionParameters() {};

PAHOMQTTConnectionParameters
PAHOMQTTConnectionParameters::get_localhost_default() {
  return PAHOMQTTConnectionParameters();
};

PAHOMQTTConnection::PAHOMQTTConnection()
    : PAHOMQTTConnection(
          PAHOMQTTConnectionParameters::get_localhost_default()) {};
PAHOMQTTConnection::PAHOMQTTConnection(
    const PAHOMQTTConnectionParameters &parameters)
    : status(PAHOMQTTConnectionStatus::DISCONNECTED),
      mqttParameters(parameters) {
  instanceCounter++;
  id = instanceCounter;
};
PAHOMQTTConnection::~PAHOMQTTConnection() {};

int PAHOMQTTConnection::getID() const { return id; }

void PAHOMQTTConnection::setConnectionParameters(
    const PAHOMQTTConnectionParameters &parameters) {
  mqttParameters = parameters;
};
const PAHOMQTTConnectionParameters &
PAHOMQTTConnection::getMQTTConnectionParameters() const {
  return mqttParameters;
};

void PAHOMQTTConnection::connect() {
  if(status == PAHOMQTTConnectionStatus::CONNECTING){
    return;
  }
  else if(status == PAHOMQTTConnectionStatus::CONNECTED){
    return;
  }
  status = PAHOMQTTConnectionStatus::CONNECTING;

  mqtt::create_options createOpts = mqtt::create_options(MQTTVERSION_5);
  createOpts.set_max_buffered_messages(mqttParameters.maxPendingMessages);
  createOpts.set_send_while_disconnected(false);

  std::string safeUri = mqttParameters.uri;
  if(safeUri.find("mqtts://") == 0){
    safeUri.replace(0, 8, "ssl://");
    std::cout << "Auto-corrected URI to: " << safeUri << std::endl;
  }

  cli = std::make_shared<mqtt::async_client>(safeUri, generateID(id), createOpts);
  mqtt::connect_options connOpts;

  connOpts.set_mqtt_version(MQTTVERSION_5);
  
  connOpts.set_clean_session(true);
  connOpts.set_keep_alive_interval(60);
  connOpts.set_automatic_reconnect(false);
  connOpts.set_max_inflight(mqttParameters.maxPendingMessages);

  if (!mqttParameters.username.empty()){
    connOpts.set_user_name(mqttParameters.username);
  }
  if (!mqttParameters.password.empty()){
    connOpts.set_password(mqttParameters.password);
  }
  if (!will.topic.empty()) {
    connOpts.set_will_message((mqtt::message_ptr)will);
  }
  if(mqttParameters.tls) {
     mqtt::ssl_options sslOpts;
     sslOpts.set_trust_store(mqttParameters.cafile);
     //sslOpts.set_key_store(mqttParameters.certfile);
     //sslOpts.set_private_key(mqttParameters.keyfile);
     connOpts.set_ssl(sslOpts);
  }

  cli->set_callback(*this);
  cli->set_disconnected_handler(std::bind(&PAHOMQTTConnection::on_disconnect, this, std::placeholders::_1, std::placeholders::_2));
  try {
      std::cout << "Sending MQTT Connect request to EMQX..." << std::endl;
      cli->connect(connOpts, nullptr, *this); 
      
  } catch (const mqtt::exception& exc) {
      std::cerr << "\n=== MQTT CONNECTION REJECTED ===" << std::endl;
      std::cerr << "Reason Code: " << exc.get_reason_code() << std::endl;
      std::cerr << "Message: " << exc.get_message() << std::endl;
      std::cerr << "What: " << exc.what() << std::endl;
      std::cerr << "================================\n" << std::endl;
  }
};

void PAHOMQTTConnection::disconnect() {
  if (cli == nullptr) {
    return;
  } else {
    try{
      cli->disconnect();
    } catch (std::exception &e) {
      printf("MQTT: got exception in disconnect: %s\n", e.what());
    }
  }
  status = PAHOMQTTConnectionStatus::DISCONNECTED;
  cli = nullptr;
};

bool PAHOMQTTConnection::send(const PAHOMQTTMessage &message) {
  if (cli == nullptr) {
    return false;
  }
  if (!cli->is_connected()) {
    return false;
  }
  if (cli->get_pending_delivery_tokens().size() >=
      mqttParameters.maxPendingMessages - 1) {
    return false;
  }
  try {
    cli->publish((mqtt::message_ptr)message);
  } catch (const std::exception &e) {
    printf("MQTT: got exception in send: %s\n", e.what());
    return false;
  }
  return true;
};

void PAHOMQTTConnection::setWillMessage(const PAHOMQTTMessage &message) {
  will = message;
};
void PAHOMQTTConnection::disableWillMessage() { will = PAHOMQTTMessage(); };

void PAHOMQTTConnection::subscribe(const std::string &topic, int qos) {
  if (cli == nullptr || status != PAHOMQTTConnectionStatus::CONNECTED) {
    return;
  }
  try {
    cli->subscribe(topic, qos);
  } catch (const std::exception &e) {
    printf("PAHOMQTTConnection: got exception in subscribe: %s\n", e.what());
  }
}

void PAHOMQTTConnection::unsubscribe(const std::string &topic) {
  if (cli == nullptr || status != PAHOMQTTConnectionStatus::CONNECTED) {
    return;
  }
  try {
    cli->unsubscribe(topic);
  } catch (const std::exception &e) {
    printf("PAHOMQTTConnection: got exception in unsubscribe: %s\n", e.what());
  }
}

void PAHOMQTTConnection::setUserData(void *userData) {
  this->userData = userData;
}
void PAHOMQTTConnection::setOnConnectCallback(on_connect_callback callback) {
  onConnectCallback = callback;
}
void PAHOMQTTConnection::setOnDisconnectCallback(
    on_disconnect_callback callback) {
  onDisconnectCallback = callback;
}
void PAHOMQTTConnection::setOnMessageCallback(on_message_callback callback) {
  onMessageCallback = callback;
}
void PAHOMQTTConnection::setOnErrorCallback(on_error_callback callback) {
  onErrorCallback = callback;
}

PAHOMQTTConnectionStatus PAHOMQTTConnection::getStatus() const {
  return status.load();
};
void PAHOMQTTConnection::on_failure(const mqtt::token &tok) {
    std::cerr << "\n=== ASYNC CONNECTION REJECTED ===" << std::endl;
    
    // The token contains the exact reason the broker dropped you
    std::cerr << "Return Code: " << tok.get_return_code() << std::endl;
    
    if (tok.get_reason_code() != 0) {
        std::cerr << "MQTT v5 Reason Code: " << tok.get_reason_code() << std::endl;
    }
    
    //std::cerr << "Message: " << tok.get_message() << std::endl;
    std::cerr << "=================================\n" << std::endl;
    status = PAHOMQTTConnectionStatus::DISCONNECTED;
};
void PAHOMQTTConnection::on_success(const mqtt::token &tok) {
    std::cout << "\n=== MQTT SUCCESSFULLY CONNECTED! ===" << std::endl;
    status = PAHOMQTTConnectionStatus::CONNECTED;
};
void PAHOMQTTConnection::connected(const std::string &cause) {
  status = PAHOMQTTConnectionStatus::CONNECTED;
  if (onConnectCallback) {
    onConnectCallback(this, userData);
  }
};
void PAHOMQTTConnection::connection_lost(const std::string &cause) {
  status = PAHOMQTTConnectionStatus::DISCONNECTED;
  if (onDisconnectCallback) {
    onDisconnectCallback(this, userData);
  }
};

void PAHOMQTTConnection::on_disconnect(const mqtt::properties &prop,
                                       mqtt::ReasonCode code) {
  status = PAHOMQTTConnectionStatus::DISCONNECTED;
  std::cerr << "\n=== MQTT DISCONNECTED ===" << std::endl;
  std::cerr << "Reason Code: " << code << std::endl;
  if (onDisconnectCallback) {
    onDisconnectCallback(this, userData);
  }
}
void PAHOMQTTConnection::delivery_complete(mqtt::delivery_token_ptr token) {};
void PAHOMQTTConnection::message_arrived(mqtt::const_message_ptr msg) {
  if (onMessageCallback) {
    onMessageCallback(this, userData, PAHOMQTTMessage(msg));
  }
};
