cmake_minimum_required(VERSION 3.16)

project(communication)

if(APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Mosquitto IMPORTED_TARGET libmosquitto REQUIRED)
endif()

find_package(Boost REQUIRED COMPONENTS
    iostreams system
)

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/websocketpp
)

get_property(DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

set(
    communication_SOURCE_DIRS
    ${DIRS}
    PARENT_SCOPE
)

include_directories(
  ${websocketpp_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${DIRS}
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
    Boost::iostreams
    Boost::system
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PkgConfig::Mosquitto)
endif()

if(NOT APPLE)
    target_link_libraries(${PROJECT_NAME} stdc++fs crypt mosquitto)
endif()

add_executable(test test/main.cpp)

target_link_libraries(test ${PROJECT_NAME})
