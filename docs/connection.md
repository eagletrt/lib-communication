# Implement New Connection Type

## Introduction

To create a new connection type you should just implement all virtual functions in the abstract `Connection` class, that is in the `inc/connection.h` file.

Every message has to be of type `Generic Message`.  
The structure of this struct is show here below:
```
struct GenericMessage {
    string topic;
    string payload;
};
```

To generalize the connection type you have to implement a class that will contain all the things you'll need for the connection, such as the context and the socket. The class you're going to create must inherit from `GenericSocket` class, a void class that has been created in the `inc/connection.h` file. Once you've done so, you can create your connection class, that will inherit from `Connection` (the abstract class mentioned above).

## Variables list

All variables are `protected` and can be accessed from the derived class. The following variables are available:

- These variables are needed to set up the connection. You can only set them by calling the `Connection`'s init function, by passing to it them all.
    ```
    string address; // FORMAT EXAMPLE: 127.0.0.1
    string port;    // FORMAT EXAMPLE: 8080
    int openMode;   // FORMAT EXAMPLE: PUB/SUB (it's an enum)
    ```

- This is the object mentioned above, where should be stored your custom socket class.
    ```
    GeneralSocket* socket;
    ```

- These variables are checked inside the Connection class, so you have to manage them.  
    For example:
    * the `open = true` when the connection has started;  
    * the `done = true` when you close the connection;  
    * `new_data` should never be modify, because is managed by the `Connection` class.

    ```
    bool done;
    bool open;
    bool new_data;
    ```

- These variables are used in the ```Connection``` class to manage the reading and the writing operation.
The ```mtx``` is used to lock the variables, so you can't read or write them at the same time; the ```cv``` is used to wait for the other thread to finish the operation; the ```buff_send``` is used to store the data that will be sent.
    ```
    mutex mtx;
    condition_variable cv;
    queue<GenerciMessage> buff_send;
    ```

- Every client has a unique id, that is stored in this variable.
    ```
    string id;
    ```

- The max queue size is determined by this variable.
    ```
    int max_queue_size;
    ```

- You can check how many subscription does a subscriber has by using this variable.
    ```
    int subscription_count;
    ```

## Methods to implement

Here below the list of all the needed methods:

- The **constructor** should only call the `Connection`'s contrsuctor.
    ```
    // in the .cpp file
    [YOUR_CLASS]::[YOUR_CLASS]() : Connection() {
        // code here will be executed before the Connection constructor
    }
    ```
- The **destructor** should only call the `Connection`'s destructor and delete all pointers and other allocated variables.
    ```
    // in the .cpp file
    [YOUR_CLASS]::~[YOUR_CLASS]() {
        // code here will be executed before the Connection destructor
        delete [VARIABLE_NAME];
    }
    ```
- The **close connection** function does not need parameter, because all connection items you'll need should be in the `GeneralSocket` item.
In this function you have to manage the connection closing.
    ```
    void closeConnection();
    ```
- The **subscribe** function should manage the subscriber's subscription to a spcified topic
    ```
    void subscribe(const string& topic);
    ```
- The **unsubscribe** function should manage the subscriber's unsubscription to a specified topic.
    ```
    void unsubscribe(const string& topic);
    ```
- The **send message** function should take a `GenericMessage` type message and **just** has to send it.
    ```
    void sendMessage(const message& msg);
    ```
- The **receive message** function should call the receive function of your connection and save the arguments in the given `message` type message. Different implementations may use callbacks, in these case you won't need this method.
    ```
    void receiveMessage(message& msg);
    ```
- The **start** function have to start either a `PUB` or a `SUB` connection and then **must** create a thread calling the `sendLoop()` function or the `receiveLoop()` function depending on the type of the client (`PUB` or `SUB`) and return the thread. It will start the connection using the parameter given to the **init** function.
    ```
    thread* start();
    ```

## Already implemented methods (in the Connection class)

##### Public methods
- The **init** function sets the connection's variables.  
As told before, to set the connection's variables you have to call this  function.
    ```
    void init(const string& address, const string& port, const int& openMode);
    ```
- The **set data** will take a `GenericMessage` type message and insert it in the queue.
    ```
    void setData(GenericMessage msg);
    ```
##### Protected methods
- This function is called by the **send loop** function.
It will loop and check if the queue is not empty, if there's at least one message, it will call your **send message** function.
    ```
    void sendLoop();
    ```
- This function is called by the **receive loop** function.
It will loop and check if there are messages to receive by calling your **receive message** function.
The received message will be used by `clbk_on_message`.
    ```
    void receiveLoop();
    ```
- This function will reset the connection's variables.
    ```
    void reset();
    ```
- This function will stop and close the connection calling your **close connection** function.
    ```
    void stop();
    ```
- This function will clear the queue.
    ```
    void clearData();
    ```
##### Callbacks
###### Set callbacks
- This function will set the given function to the `clbk_on_connect` variable.
```
void add_on_connect(function<void(const int &id)>);
```
- This function will set the given function to the `clbk_on_disconnect` variable.
```
void add_on_disconnect(function<void(const int &id, const int &code)>);
```
- This function will set the given function to the `clbk_on_error` variable.
```
void add_on_error(function<void(const int &id, const int &code, const string &msg)>);
```
- This function will set the given function to the `clbk_on_message` variable.
```
void add_on_message(function<void(const int &id, const GenericMessage &msg)>);
```
- This function will set the given function to the `clbk_on_subscribe` variable.
```
void add_on_subscribe(function<void(const int &id, const string &topic)>);
```
- This function will set the given function to the `clbk_on_unsubscribe` variable.
```
void add_on_unsubscribe(function<void(const int &id, const string &topic)>);
```
###### Use callbacks
- This function will call the `clbk_on_connect` callback.
It have to be called when the connection is opened.
    ```
    void onConnect(const int &id);
    ```
- This function will call the `clbk_on_disconnect` callback.
It have to be called when the connection is closed (also when fatal errors occurs).
The given code should be 0 if there's no error, otherwise it should be the error code.
    ```
    void onDisconnect(const int &id, const int& code);
    ```
- This function will call the `clbk_on_error` callback.
It have to be called when an error occurs (remember to handle errors).
It needs the error code and the error message.
    ```
    void onError(const int &id, const int& code, const string& msg);
    ```
- This function will call the `clbk_on_message` callback.
It's used in the **receive loop** function.
It takes the received message as a parameter.
    ```
    void onMessage(const int &id, const GenericMessage& msg);
    ```
- This function will call the `clbk_on_subscribe` callback.
It has to be called when a subscription is done.
It takes the topic as a parameter.
    ```
    void onSubscribe(const int &id, const string& topic);
    ```
- This function will call the `clbk_on_unsubscribe` callback.
It have to be called when an unsubscription is done.
It take the topic as a parameter.
    ```
    void onUnsubscribe(const int &id, const string& topic);
    ```
- This function will call the `clbk_on_publish` callback.
It have to be called when a message is published.
It takes the topic as a parameter.
    ```
    void onPublish(const int &id, const string& topic);
    ```

## Usage

### Create a connection
1. Create a connection object.
It will init connection variables.
    ```
    // the name should represent either the publisher or the subscriber
    [YOUR_CLASS] name;
    ```
2. Init the connection.
    ```
    name.init("localhost", "1883", PUB);

    // or

    name.init("localhost", "1883", SUB);
    ```
3. Set up the callbacks.
    ```
    // where [CALLBACK] is either open, close, error, message, subscribe, unsubscribe
    name.addOn[CALLBACK]([&]() {
        // do something
    });
    ```
4. Start the connection.
It will automatically run the **send loop** or the **receive loop**, depending on the connection's mode setted in the **init** function.
    ```
    thread* thread_name = name.start();
    ```
5. Subscribe/unsubscribe to/from a topic or set data.
    ```
    name.subscribe("topic");

    // or

    name.unsubscribe("topic");

    // or

    // the topic should be the ID of the message
    name.setData("topic", "data");
    ```
6. Close the connection.
    ```
    name.closeConnection();
    ```

### Examples
An example of the connection could be seen at `src/connection/mqtt_connection.cpp`, with the associated header at `inc/connection/mqtt_connection.h`.  
A working test example could be seen at `scripts/testMQTT/test.cpp`. It's a simple connection that will publish and subscribe to a topic.  
The example describes an MQTT working connection between a publisher and a subscriber comunicating between two different threads.
Another example can be seen at `src/connection/ws_connection.cpp` with its header at `src/connection/ws_connection.cpp` and the test at `scripts/testWS/test.cpp`. It's a simple WebSocket connection that will publish and subscribe to a topic.