cmake_minimum_required(VERSION 3.16)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# WebSocket client example
add_executable(websocket_client_example websocket_client_example.cpp)
target_link_libraries(websocket_client_example
    ocpp-gateway-common
    ocpp-gateway-ocpp
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# Mapping hot reload example
add_executable(mapping_hot_reload_example mapping_hot_reload_example.cpp)
target_link_libraries(mapping_hot_reload_example
    ocpp-gateway-common
    ocpp-gateway-ocpp
    ${Boost_LIBRARIES}
    yaml-cpp
    ${JSONCPP_LIBRARIES}
    Threads::Threads
)