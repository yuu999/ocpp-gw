set(TEST_SOURCES
    # Add integration test source files here as they are created
    # Example: test_ocpp_client_integration.cpp
)

# Create test executable if there are test sources
if(TEST_SOURCES)
    add_executable(integration_tests ${TEST_SOURCES})

    # Link with Google Test and project libraries
    target_link_libraries(integration_tests
        gtest
        gtest_main
        ocpp-gateway-common
        ocpp-gateway-mapping
        ocpp-gateway-device
        ocpp-gateway-ocpp
        ocpp-gateway-api
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
    gtest_discover_tests(integration_tests)
endif()