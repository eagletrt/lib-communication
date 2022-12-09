# MOSQUITTO SETUP

## INDEX:
- [Installation](#installation)
    - [Client](#client)
    - [Broker](#broker)
        - [Test the Broker](#test-the-broker)
- [Compile and Test](#compile--test)
- [Usage](#usage)
    - [Broker Configuration File](#brokers-configuration-file)
    - [Start the Broker](#start-the-broker)
    - [Start the Telemetry](#start-the-telemetry)
    - [Setup the Connection](#setup-the-connection)

## INSTALLATION:
### CLIENT
First of all you have to install mosquitto library on the device where
you'll run the telemetry, using
```
sudo apt install libmosquitto-dev
sudo apt install mosquitto-dev
```

### BROKER
On the broker device you have to install the mosquitto broker service by
```
sudo snap install mosquitto
```

or, if you'll run the broker on 32-bit devices or if you don't have snap installed
```
sudo apt install mosquitto
```

#### TEST THE BROKER
Before using the telemetry test if the broker work by installing also
`mosquito-clients`, so
```
sudo snap install mosquitto-clients
```
or, as for the broker installation,
```
sudo apt install mosquitto-clients
```
Once you had installed `mosquitto-clients`, you can run a
`subscriber` by
```
mosquitto_sub -h localhost -p 1883 -t [TOPIC_TO_SUBSCRIBE] {-u [USERNAME] -P [PASSWORD]}
```
and, on another terminal, run a `publisher` by
```
mosquitto_pub -h localhost -p 1883 -t [TOPIC_TO_PUBLISH] -m [MESSAGE] {-u [USERNAME] -P [PASSWORD]}
```
where the `username` and the `password` had been set on the broker (not
mandatory, but raccommended). If there's no users or if you want to add
some, you can follow the [Documentation](https://mosquitto.org/man/mosquitto_passwd-1.html).

>#### EXAMPLE:
>Create a password's file `passwords` and run
>```
>sudo mosquitto_passwd -c [FILE_NAME] [USERNAME]
>```
>You'll be asked to choose a password and to repeat it.

## COMPILE & TEST:
To use the mosquitto library you also have to append the
`-lmosquitto` flag.
There's a test in `scripts/testMQTT` folder.

## USAGE:
### Broker's Configuration File
You can find the `.conf` file in the `/etc/mosquitto/` folder.
```
# Place your local configuration in /etc/mosquitto/conf.d/
#
# A full description of the configuration file is at
# /usr/share/doc/mosquitto/examples/mosquitto.conf.example

#allow_anonymous true

listener 1883 0.0.0.0

pid_file /run/mosquitto/mosquitto.pid

persistence true
persistence_location /var/lib/mosquitto/

log_dest topic
log_type error
log_type warning
log_type notice
log_type information
connection_messages true
log_timestamp true

password_file /etc/mosquitto/passwords

log_dest file /var/log/mosquitto/mosquitto.log

include_dir /etc/mosquitto/conf.d
```
- `allow_anonymous true`: can be useful while testing, to avoid creating
new users or to don't add corresponging flags while executing the code.
- `listener 1883 0.0.0.0`: must be insert to make the broker listen on
all its addresses.
- `include_dir /etc/mosquitto/conf.d`: it's the folder where you can
add your custom configuration files.
- `log_dest file /var/log/mosquitto/mosquitto.log`: here you can find
the log about your last broker session.

### Start the Broker
Before starting the broker, remember to stop the service that is enabled
by default on the device. To do so, you can run
```
sudo systemctl stop mosquitto.service
```
After the setup, you can start the broker by
```
./run_broker.sh
```
that will execute the following line
```
sudo mosquitto --verbose --config-file ./mosquitto.conf
```
If you don't have the `./run_broker.sh` file you can write it by yourself.

If you would like to modify the configuration of the broker just add a
new `.conf` file inside the `conf.d/` folder.

For the documentation about writing `.conf` files visit the
[Mosquitto Documentation](https://mosquitto.org/man/mosquitto-conf-5.html).

### Start the Telemetry
For this part just have a look at the [Telemetry Documentation](https://github.com/eagletrt/telemetry/tree/main/docs/index.md)
and, if you want to create a new type of connection, at the
[Connection Documentation](connection.md).

### Setup the Connection
Before setting up the connection, here's a thing you have to know:
the topic field can be used to create a tree of topics, so you can
subscribe to a topic and receive all the messages published on that
topic and on all its subtopics. For example, if you subscribe to the
topic `home/sensors/temperature` you'll receive all the messages
published on `home/sensors/temperature`, `home/sensors/temperature/1`
and `home/sensors/temperature/2`.
1. To setup the connection you have to create a new `MQTTConnection`
object and, if needed, pass the `username` and the `password` to it.

2. Add all callbacks you want to use to the `MQTTConnection` object by using the `addOn[CALLBACK]` method.

3. Init the connection by calling the `init` method
(view [Connection Documentation](connection.md) to see which
parameters it will need and for other information about connections).

4. Start the connection by calling the `start` method and storing the
return thread.

5. Subscribe/unsubscribe to/from a topic or set data.
    ```
    name.subscribe("topic");

    // or

    name.unsubscribe("topic");

    // or

    name.setData("topic", "data");
    ```

6. Close the connection.
    ```
    name.closeConnection();
    ```

## EXAMPLES:
The MQTT example is in the `script/testMQTT/` folder.
The `.cpp` and the `.h` files are in the `src/connection/` and in the
`inc/connection` folder. The user must check the pending messages
counter before publishing by calling `getPendingMessages()`.
