# ソースファイルの設定
set(COMMON_SOURCES
    common/logger.cpp
    common/config_manager.cpp
    common/file_watcher.cpp
    common/device_config.cpp
    common/csms_config.cpp
    common/system_config.cpp
    common/metrics_collector.cpp
    common/prometheus_exporter.cpp
)

# APIライブラリ
set(API_SOURCES
    api/admin_api.cpp
    api/cli_manager.cpp
    api/web_ui.cpp
)

# デバイス通信ライブラリ
set(DEVICE_SOURCES
    device/device_adapter.cpp
    device/modbus_rtu_adapter.cpp
    device/modbus_tcp_adapter.cpp
    device/echonet_lite_adapter.cpp
)

# OCPP通信ライブラリ
set(OCPP_SOURCES
    ocpp/ocpp_client_manager.cpp
    ocpp/ocpp_message_handlers.cpp
    ocpp/ocpp_message_processor.cpp
    ocpp/websocket_client.cpp
    ocpp/message.cpp
    ocpp/boot_notification.cpp
    ocpp/heartbeat.cpp
    ocpp/status_notification.cpp
    ocpp/transaction_event.cpp
    ocpp/message_factory.cpp
    ocpp/evse_state_machine.cpp
    ocpp/mapping_config.cpp
    ocpp/variable_translator.cpp
)

# 共通ライブラリ
add_library(ocpp-gateway-common STATIC ${COMMON_SOURCES})
target_link_libraries(ocpp-gateway-common
    ${Boost_LIBRARIES}
    yaml-cpp
    ${JSONCPP_LIBRARIES}
    spdlog::spdlog
    Threads::Threads
)

# APIライブラリ
add_library(ocpp-gateway-api STATIC ${API_SOURCES})
target_link_libraries(ocpp-gateway-api
    ocpp-gateway-common
    ${Boost_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    Threads::Threads
)

# デバイス通信ライブラリ
add_library(ocpp-gateway-device STATIC ${DEVICE_SOURCES})
target_link_libraries(ocpp-gateway-device
    ocpp-gateway-common
    ${Boost_LIBRARIES}
    ${LIBMODBUS_LIBRARIES}
    Threads::Threads
)

# OCPP通信ライブラリ
add_library(ocpp-gateway-ocpp STATIC ${OCPP_SOURCES})
target_link_libraries(ocpp-gateway-ocpp
    ocpp-gateway-common
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${JSONCPP_LIBRARIES}
    Threads::Threads
)

# メイン実行ファイル
add_executable(ocpp-gateway main.cpp)
target_link_libraries(ocpp-gateway
    ocpp-gateway-common
    ocpp-gateway-api
    ocpp-gateway-device
    ocpp-gateway-ocpp
    ${Boost_LIBRARIES}
    Threads::Threads
)

# コンパイル定義
target_compile_definitions(ocpp-gateway-common PRIVATE
    BOOST_ALL_NO_LIB
    BOOST_ASIO_SEPARATE_COMPILATION
)

# インクルードディレクトリの設定
target_include_directories(ocpp-gateway-common PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
    ${Boost_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${LIBMODBUS_INCLUDE_DIRS}
)

# 各ライブラリに同じインクルードディレクトリを設定
foreach(lib ocpp-gateway-api ocpp-gateway-device ocpp-gateway-ocpp ocpp-gateway)
    target_include_directories(${lib} PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party
        ${Boost_INCLUDE_DIRS}
        ${JSONCPP_INCLUDE_DIRS}
        ${LIBMODBUS_INCLUDE_DIRS}
    )
endforeach() 