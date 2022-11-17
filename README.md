# lib-communication
C++ library with implementations of connection, websocket and mqtt connection

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