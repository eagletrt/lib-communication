cmake_minimum_required(VERSION 3.16)

project(communication)

option(TEST "Build test" OFF)

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

if(APPLE)
    message(STATUS "Boost_USE_STATIC_LIBS: ${Boost_USE_STATIC_LIBS}")
    if(${Boost_USE_STATIC_LIBS})
        message(STATUS "Using static boost libraries, including static zstd library")
        find_package(zstd REQUIRED)
        # message(STATUS "zstd_STATIC_LIB: ${zstd_STATIC_LIB}")
        target_link_libraries(${PROJECT_NAME} zstd::libzstd_static)
    endif()

    target_link_libraries(${PROJECT_NAME} PkgConfig::Mosquitto)
endif()

target_link_libraries(
    ${PROJECT_NAME}
    ${websocketpp}
    Boost::iostreams
    Boost::system
)

if(NOT APPLE)
    target_link_libraries(${PROJECT_NAME} stdc++fs crypt mosquitto)
endif()

if(TEST)
    enable_testing()
    add_subdirectory(test)
    add_executable(test test/main.cpp)

    target_link_libraries(test ${PROJECT_NAME})
endif()
