set(TEST_SOURCES
    # Basic test files that exist
    test_sample.cpp
)

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
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# Add libmodbus if available
if(LIBMODBUS_FOUND)
    target_link_libraries(unit_tests ${LIBMODBUS_LIBRARIES})
endif()

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)
