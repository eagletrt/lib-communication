cmake_minimum_required(VERSION 3.16)

project(communication)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(TEST "Build test" OFF)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread -O0 -g")

if(APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Mosquitto IMPORTED_TARGET libmosquitto REQUIRED)
endif()

find_package(Boost REQUIRED COMPONENTS
    iostreams system
)

include_directories(
  ${websocketpp_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

set(SOURCES
    src/connection.cpp
    src/mqtt_connection.cpp
)

get_property(DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

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
)

if(NOT APPLE)
    target_link_libraries(${PROJECT_NAME} mosquitto)
endif()

if(TEST)
    add_executable(test_1 test/main.cpp)
    target_link_libraries(test_1 ${PROJECT_NAME})
    add_executable(pubber test/pubber.cpp)
    target_link_libraries(pubber ${PROJECT_NAME})
endif()
