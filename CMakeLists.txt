cmake_minimum_required(VERSION 3.16)

project(communication)

find_package(Boost REQUIRED COMPONENTS
    iostreams system
)

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/websocketpp
)

include_directories(
  ${websocketpp_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

set(SOURCES
    src/connection.cpp
    src/mqtt_connection.cpp
    src/ws_connection.cpp
)

add_library(
    ${PROJECT_NAME}
    STATIC
    ${SOURCES}
)

target_link_libraries(
    ${PROJECT_NAME}
    ${websocketpp}
    mosquitto
    Boost::iostreams
    Boost::system
)

add_executable(test test/main.cpp)

target_link_libraries(${PROJECT_NAME})

