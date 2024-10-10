cmake_minimum_required(VERSION 3.16)

project(communication)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(TEST "Build test" OFF)

if(APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Mosquitto IMPORTED_TARGET libmosquitto REQUIRED)
endif()

include_directories(
    SYSTEM
  ${CMAKE_CURRENT_LIST_DIR}/thirdparty
  ${CMAKE_CURRENT_LIST_DIR}/inc
  ${CMAKE_CURRENT_LIST_DIR}/external/paho/include
)

set(PAHO_WITH_SSL OFF)
set(PAHO_ENABLE_TESTING OFF)
set(PAHO_HIGH_PERFORMANCE ON)
option(PAHO_WITH_MQTT_C "Build Paho C from the internal GIT submodule." TRUE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/paho)
target_compile_options(paho-mqttpp3 PRIVATE -w)

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/connection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/mqtt_connection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/paho_mqtt_connection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/connection_manager.cpp
)

get_property(DIRS DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY INCLUDE_DIRECTORIES)

set(
    communication_SOURCE_DIRS
    ${DIRS}
    PARENT_SCOPE
)

add_library(
    ${PROJECT_NAME}
    STATIC
    ${SOURCES}
)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    paho-mqttpp3
    paho-mqtt3c
)

if (APPLE)
    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::Mosquitto)
endif ()

if(NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PUBLIC mosquitto)
endif()

if(TEST)
    add_executable(test_1 test/main.cpp)
    target_link_libraries(test_1 ${PROJECT_NAME} pthread)
    add_executable(pubber test/pubber.cpp)
    target_link_libraries(pubber ${PROJECT_NAME} pthread)
    add_executable(subber test/subber.cpp)
    target_link_libraries(subber ${PROJECT_NAME} pthread)
    add_executable(paho_test test/paho_test.cpp)
    target_link_libraries(paho_test ${PROJECT_NAME})
endif()
