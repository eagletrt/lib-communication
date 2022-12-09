# lib-communication
C++ library with implementations of connection, websocket and mqtt connection

## Other docs:
- [Implement New Connection Type](docs/connection.md)
    1. [Introduction](docs/connection.md#Introduction)
    2. [Variables list](docs/connection.md#variables-list)
    3. [Methods to implement](docs/connection.md#methods-to-implement)
    4. [Already implemented methods (in the Connection class)](docs/connection.md#already-implemented-methods-in-the-connection-class)
        - [Public methods](docs/connection.md#public-methods)
        - [Protected methods](docs/connection.md#protected-methods)
        - [Callbacks](docs/connection.md#callbacks)
    5. [Usage](docs/connection.md#usage)
        - [Create a connection](docs/connection.md#create-a-connection)
        - [Examples](docs/connection.md#examples)
- [Mosquitto Setup](docs/mqtt_connection.md)
    - [Installation](docs/mqtt_connection.md#installation)
        - [Client](docs/mqtt_connection.md#client)
        - [Broker](docs/mqtt_connection.md#broker)
            - [Test the Broker](docs/mqtt_connection.md#test-the-broker)
    - [Compile and Test](docs/mqtt_connection.md#compile--test)
    - [Usage](docs/mqtt_connection.md#usage)
        - [Broker's Configuration File](docs/mqtt_connection.md#brokers-configuration-file)
        - [Start the Broker](docs/mqtt_connection.md#start-the-broker)
        - [Start the Telemetry](docs/mqtt_connection.md#start-the-telemetry)
        - [Setup the Connection](docs/mqtt_connection.md#setup-the-connection)
    - [Examples](docs/mqtt_connection.md#examples)

## How to use
```Cmake
add_subdirectory(*path-to-lib-communication*)

include_directories(
    ${communication_SOURCE_DIRS}
)

target_link_libraries(
    *your-target*
    communication
)
```