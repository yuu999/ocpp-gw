set(TEST_SOURCES
    # Add unit test source files here as they are created
    ocpp/message_test.cpp
    ocpp/websocket_client_test.cpp
    ocpp/ocpp_client_manager_test.cpp
    ocpp/ocpp_client_manager_evse_test.cpp
    ocpp/ocpp_message_processor_test.cpp
    ocpp/evse_state_machine_test.cpp
    ocpp/mapping_config_test.cpp
    ocpp/mapping_config_hot_reload_test.cpp
    ocpp/variable_translator_test.cpp
    common/config_test.cpp
    common/logger_test.cpp
    common/metrics_collector_test.cpp
    common/metrics_collector_test_extended.cpp
    device/device_adapter_test.cpp
    device/echonet_lite_adapter_test.cpp
    device/modbus_rtu_adapter_test.cpp
    api/admin_api_test.cpp
    api/admin_api_test_extended.cpp
    api/cli_manager_test.cpp
    api/web_ui_test.cpp
)

# Create a sample test file to demonstrate Google Test usage
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/test_sample.cpp "
#include <gtest/gtest.h>

// Sample test to verify Google Test setup
TEST(SampleTest, BasicAssertions) {
    // Expect equality
    EXPECT_EQ(2 + 2, 4);
    
    // Expect inequality
    EXPECT_NE(2 + 2, 5);
    
    // Expect true
    EXPECT_TRUE(1 == 1);
    
    // Expect false
    EXPECT_FALSE(1 == 2);
}
")

# Add the sample test to the test sources
set(TEST_SOURCES ${TEST_SOURCES} test_sample.cpp)

# Create test executable
add_executable(unit_tests ${TEST_SOURCES})

# Link with Google Test and project libraries
target_link_libraries(unit_tests
    gtest
    gtest_main
    ocpp-gateway-common
    ocpp-gateway-api
    ocpp-gateway-device
    ocpp-gateway-mapping
    ocpp-gateway-ocpp
    ${Boost_LIBRARIES}
    yaml-cpp
    ${JSONCPP_LIBRARIES}
    ${LIBMODBUS_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)
