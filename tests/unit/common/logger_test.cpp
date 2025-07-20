#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/error.h"

namespace ocpp_gateway {
namespace common {
namespace test {

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for log files
        temp_dir_ = std::filesystem::temp_directory_path() / "ocpp_gateway_test_logs";
        std::filesystem::create_directories(temp_dir_);
        log_file_ = (temp_dir_ / "test.log").string();
    }

    void TearDown() override {
        // Clean up temporary directory
        std::filesystem::remove_all(temp_dir_);
    }

    std::filesystem::path temp_dir_;
    std::string log_file_;
};

TEST_F(LoggerTest, BasicInitialization) {
    // Test basic initialization
    EXPECT_TRUE(Logger::initialize("debug", log_file_));
    
    // Test logging
    auto logger = Logger::get();
    ASSERT_NE(logger, nullptr);
    
    logger->debug("Debug message");
    logger->info("Info message");
    logger->warn("Warning message");
    logger->error("Error message");
    
    // Flush to ensure messages are written
    Logger::flush();
    
    // Verify log file exists
    EXPECT_TRUE(std::filesystem::exists(log_file_));
    
    // Verify file has content
    std::ifstream log_stream(log_file_);
    std::string content((std::istreambuf_iterator<char>(log_stream)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_FALSE(content.empty());
    EXPECT_TRUE(content.find("Debug message") != std::string::npos);
    EXPECT_TRUE(content.find("Info message") != std::string::npos);
    EXPECT_TRUE(content.find("Warning message") != std::string::npos);
    EXPECT_TRUE(content.find("Error message") != std::string::npos);
}

TEST_F(LoggerTest, LogLevelControl) {
    // Initialize with info level
    EXPECT_TRUE(Logger::initialize("info", log_file_));
    
    auto logger = Logger::get();
    ASSERT_NE(logger, nullptr);
    
    // Debug should not be logged at info level
    logger->debug("Debug message");
    logger->info("Info message");
    Logger::flush();
    
    std::ifstream log_stream(log_file_);
    std::string content((std::istreambuf_iterator<char>(log_stream)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_FALSE(content.find("Debug message") != std::string::npos);
    EXPECT_TRUE(content.find("Info message") != std::string::npos);
    
    // Change level to debug
    Logger::setLevel("debug");
    logger->debug("Debug message after level change");
    Logger::flush();
    
    log_stream = std::ifstream(log_file_);
    content = std::string((std::istreambuf_iterator<char>(log_stream)),
                         std::istreambuf_iterator<char>());
    
    EXPECT_TRUE(content.find("Debug message after level change") != std::string::npos);
    
    // Test enum-based level setting
    Logger::setLevel(LogLevel::ERROR);
    logger->warn("Warning should not appear");
    logger->error("Error should appear");
    Logger::flush();
    
    log_stream = std::ifstream(log_file_);
    content = std::string((std::istreambuf_iterator<char>(log_stream)),
                         std::istreambuf_iterator<char>());
    
    EXPECT_FALSE(content.find("Warning should not appear") != std::string::npos);
    EXPECT_TRUE(content.find("Error should appear") != std::string::npos);
}

TEST_F(LoggerTest, LogRotation) {
    // Set up a small max size to test rotation
    LogConfig config;
    config.log_level = "debug";
    config.log_file = log_file_;
    config.max_size_mb = 1; // 1MB
    config.max_files = 3;
    config.compress_logs = false; // Disable compression for this test
    
    EXPECT_TRUE(Logger::initialize(config));
    
    auto logger = Logger::get();
    ASSERT_NE(logger, nullptr);
    
    // Write enough data to trigger rotation
    std::string large_message(100000, 'X'); // 100KB message
    for (int i = 0; i < 15; i++) { // Should create ~1.5MB of logs
        logger->info("Large message {}: {}", i, large_message);
    }
    
    Logger::flush();
    
    // Check that rotation files exist
    EXPECT_TRUE(std::filesystem::exists(log_file_));
    EXPECT_TRUE(std::filesystem::exists(log_file_ + ".1"));
    
    // The exact number of rotation files depends on the actual size,
    // but we should have at least one rotation file
    bool has_rotation = std::filesystem::exists(log_file_ + ".1");
    EXPECT_TRUE(has_rotation);
}

TEST_F(LoggerTest, LogCompression) {
    // Create a test log file
    std::string test_log = (temp_dir_ / "compress_test.log").string();
    std::ofstream test_file(test_log);
    test_file << "Test log content for compression" << std::endl;
    test_file.close();
    
    // Compress logs
    int compressed = Logger::compressOldLogs(temp_dir_.string(), "*.log");
    
    // Check that compression worked
    EXPECT_GE(compressed, 1);
    EXPECT_TRUE(std::filesystem::exists(test_log + ".gz"));
    EXPECT_FALSE(std::filesystem::exists(test_log)); // Original should be removed
}

TEST_F(LoggerTest, LogMacros) {
    EXPECT_TRUE(Logger::initialize("debug", log_file_));
    
    // Test logging macros
    LOG_DEBUG("Debug macro test");
    LOG_INFO("Info macro test");
    LOG_WARN("Warning macro test");
    LOG_ERROR("Error macro test");
    
    Logger::flush();
    
    std::ifstream log_stream(log_file_);
    std::string content((std::istreambuf_iterator<char>(log_stream)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_TRUE(content.find("Debug macro test") != std::string::npos);
    EXPECT_TRUE(content.find("Info macro test") != std::string::npos);
    EXPECT_TRUE(content.find("Warning macro test") != std::string::npos);
    EXPECT_TRUE(content.find("Error macro test") != std::string::npos);
}

class ErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger for error tests
        temp_dir_ = std::filesystem::temp_directory_path() / "ocpp_gateway_error_test";
        std::filesystem::create_directories(temp_dir_);
        log_file_ = (temp_dir_ / "error_test.log").string();
        Logger::initialize("debug", log_file_);
    }

    void TearDown() override {
        // Clean up temporary directory
        std::filesystem::remove_all(temp_dir_);
    }

    std::filesystem::path temp_dir_;
    std::string log_file_;
};

TEST_F(ErrorHandlingTest, ExceptionHierarchy) {
    // Test that exceptions inherit correctly
    ConfigValidationError config_error("Config error");
    EXPECT_TRUE(dynamic_cast<OcppGatewayException*>(&config_error) != nullptr);
    
    NetworkError network_error("Network error");
    EXPECT_TRUE(dynamic_cast<OcppGatewayException*>(&network_error) != nullptr);
    
    ProtocolError protocol_error("Protocol error");
    EXPECT_TRUE(dynamic_cast<OcppGatewayException*>(&protocol_error) != nullptr);
    
    DeviceError device_error("Device error");
    EXPECT_TRUE(dynamic_cast<OcppGatewayException*>(&device_error) != nullptr);
    
    TimeoutError timeout_error("Timeout error");
    EXPECT_TRUE(dynamic_cast<OcppGatewayException*>(&timeout_error) != nullptr);
    
    SecurityError security_error("Security error");
    EXPECT_TRUE(dynamic_cast<OcppGatewayException*>(&security_error) != nullptr);
    
    InternalError internal_error("Internal error");
    EXPECT_TRUE(dynamic_cast<OcppGatewayException*>(&internal_error) != nullptr);
}

TEST_F(ErrorHandlingTest, ExceptionMessages) {
    // Test exception messages
    ConfigValidationError config_error("Invalid configuration");
    EXPECT_STREQ(config_error.what(), "Invalid configuration");
    
    NetworkError network_error("Connection failed");
    EXPECT_STREQ(network_error.what(), "Connection failed");
    
    // Test with error code
    std::error_code ec(ECONNREFUSED, std::system_category());
    NetworkError network_error_with_code("Connection refused", ec);
    EXPECT_TRUE(std::string(network_error_with_code.what()).find("Connection refused") != std::string::npos);
    EXPECT_EQ(network_error_with_code.error_code(), ec);
    
    // Test with device error code
    DeviceError device_error("Device communication error", 42);
    EXPECT_TRUE(std::string(device_error.what()).find("Device communication error") != std::string::npos);
    EXPECT_TRUE(std::string(device_error.what()).find("42") != std::string::npos);
    EXPECT_EQ(device_error.device_error_code(), 42);
}

TEST_F(ErrorHandlingTest, ErrorUtilsFunctions) {
    // Test ErrorUtils functions
    std::error_code ec(ETIMEDOUT, std::system_category());
    
    // Test makeNetworkError
    NetworkError network_error = ErrorUtils::makeNetworkError(ec, "Operation timed out");
    EXPECT_TRUE(std::string(network_error.what()).find("Operation timed out") != std::string::npos);
    EXPECT_EQ(network_error.error_code(), ec);
    
    // Test makeDeviceError
    DeviceError device_error = ErrorUtils::makeDeviceError(123, "Device error");
    EXPECT_TRUE(std::string(device_error.what()).find("Device error") != std::string::npos);
    EXPECT_EQ(device_error.device_error_code(), 123);
    
    // Test checkOperation
    EXPECT_NO_THROW(ErrorUtils::checkOperation(true, "This should not throw"));
    EXPECT_THROW(ErrorUtils::checkOperation(false, "Operation failed"), OcppGatewayException);
    
    // Test checkNotNull
    int value = 42;
    int* ptr = &value;
    int* null_ptr = nullptr;
    EXPECT_NO_THROW(ErrorUtils::checkNotNull(ptr, "This should not throw"));
    EXPECT_THROW(ErrorUtils::checkNotNull(null_ptr, "Null pointer"), OcppGatewayException);
}

TEST_F(ErrorHandlingTest, ExceptionCatching) {
    // Test catching exceptions
    try {
        throw NetworkError("Network connection failed");
    } catch (const NetworkError& e) {
        EXPECT_TRUE(std::string(e.what()).find("Network connection failed") != std::string::npos);
    } catch (...) {
        FAIL() << "Wrong exception type caught";
    }
    
    // Test catching base class
    try {
        throw DeviceError("Device error", 404);
    } catch (const OcppGatewayException& e) {
        EXPECT_TRUE(std::string(e.what()).find("Device error") != std::string::npos);
    } catch (...) {
        FAIL() << "Wrong exception type caught";
    }
    
    // Test exception propagation
    auto throw_func = []() {
        throw TimeoutError("Operation timed out");
    };
    
    EXPECT_THROW(throw_func(), TimeoutError);
    EXPECT_THROW(throw_func(), OcppGatewayException);
    EXPECT_THROW(throw_func(), std::exception);
}

} // namespace test
} // namespace common
} // namespace ocpp_gateway