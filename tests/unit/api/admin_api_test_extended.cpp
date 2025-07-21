#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ocpp_gateway/api/admin_api.h"
#include "ocpp_gateway/common/config_manager.h"
#include <thread>
#include <chrono>
#include <json/json.h>

using namespace ocpp_gateway::api;
using namespace ocpp_gateway::config;
using namespace testing;

// Mock ConfigManager for testing
class MockConfigManager : public ConfigManager {
public:
    MOCK_METHOD(bool, reloadAllConfigs, (), (override));
    MOCK_METHOD(SystemConfig&, getSystemConfig, (), (override));
    MOCK_METHOD(CsmsConfig&, getCsmsConfig, (), (override));
    MOCK_METHOD(DeviceConfigs&, getDeviceConfigs, (), (override));
    MOCK_METHOD(std::optional<DeviceConfig>, getDeviceConfig, (const std::string&), (override));
};

class AdminApiExtendedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a test port to avoid conflicts
        admin_api_ = std::make_unique<AdminApi>(9998, "127.0.0.1");
    }

    void TearDown() override {
        if (admin_api_) {
            admin_api_->stop();
        }
    }

    std::unique_ptr<AdminApi> admin_api_;
};

// Test creating error responses
TEST_F(AdminApiExtendedTest, CreateErrorResponseTest) {
    HttpResponse response = admin_api_->createErrorResponse(404, "Not Found");
    
    EXPECT_EQ(response.status_code, 404);
    EXPECT_EQ(response.content_type, "application/json");
    EXPECT_TRUE(response.headers.find("Access-Control-Allow-Origin") != response.headers.end());
    EXPECT_EQ(response.headers["Access-Control-Allow-Origin"], "*");
    
    // Parse the JSON response
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(response.body, root));
    
    EXPECT_TRUE(root["error"].asBool());
    EXPECT_EQ(root["message"].asString(), "Not Found");
    EXPECT_EQ(root["status_code"].asInt(), 404);
    EXPECT_TRUE(root.isMember("timestamp"));
}

// Test creating JSON responses
TEST_F(AdminApiExtendedTest, CreateJsonResponseTest) {
    std::string test_json = "{\"test\": true, \"value\": 123}";
    HttpResponse response = admin_api_->createJsonResponse(200, test_json);
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "application/json");
    EXPECT_EQ(response.body, test_json);
    EXPECT_TRUE(response.headers.find("Access-Control-Allow-Origin") != response.headers.end());
    EXPECT_EQ(response.headers["Access-Control-Allow-Origin"], "*");
}

// Test authentication
TEST_F(AdminApiExtendedTest, AuthenticationTest) {
    // Enable authentication
    admin_api_->setAuthentication(true, "test_user", "test_pass");
    
    // Create a request with no authentication
    HttpRequest request;
    
    // Create a request with invalid authentication
    HttpRequest invalid_request;
    invalid_request.headers["Authorization"] = "Basic invalid_token";
    
    // Create a request with valid authentication (simplified for testing)
    HttpRequest valid_request;
    valid_request.headers["Authorization"] = "Basic test_user:test_pass";
    
    // Test authentication (indirectly through handleRequest)
    // Note: We can't directly test authenticate() as it's private
    // This is a limitation of the current design
}

// Test route registration and handling
TEST_F(AdminApiExtendedTest, RouteRegistrationTest) {
    // Register a test route
    bool route_called = false;
    admin_api_->registerRoute(HttpMethod::GET, "/test", 
        [&route_called](const HttpRequest& req) {
            route_called = true;
            HttpResponse response;
            response.status_code = 200;
            response.body = "Test route called";
            return response;
        });
    
    // We can't directly test handleRequest as it's private
    // This is a limitation of the current design
}

// Test system info handler
TEST_F(AdminApiExtendedTest, HandleGetSystemInfoTest) {
    HttpRequest request;
    HttpResponse response = admin_api_->handleGetSystemInfo(request);
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "application/json");
    
    // Parse the JSON response
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(response.body, root));
    
    EXPECT_TRUE(root.isMember("name"));
    EXPECT_TRUE(root.isMember("version"));
    EXPECT_TRUE(root.isMember("build_date"));
    EXPECT_TRUE(root.isMember("build_time"));
    EXPECT_TRUE(root.isMember("uptime_seconds"));
}

// Test devices list handler
TEST_F(AdminApiExtendedTest, HandleGetDevicesTest) {
    HttpRequest request;
    HttpResponse response = admin_api_->handleGetDevices(request);
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "application/json");
    
    // Parse the JSON response
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(response.body, root));
    
    EXPECT_TRUE(root.isMember("devices"));
    EXPECT_TRUE(root.isMember("total"));
    EXPECT_TRUE(root["devices"].isArray());
}

// Test health check handler
TEST_F(AdminApiExtendedTest, HandleGetHealthTest) {
    HttpRequest request;
    HttpResponse response = admin_api_->handleGetHealth(request);
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "application/json");
    
    // Parse the JSON response
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(response.body, root));
    
    EXPECT_TRUE(root.isMember("status"));
    EXPECT_EQ(root["status"].asString(), "healthy");
    EXPECT_TRUE(root.isMember("timestamp"));
    EXPECT_TRUE(root.isMember("uptime_seconds"));
    EXPECT_TRUE(root.isMember("running"));
}

// Test reload config handler
TEST_F(AdminApiExtendedTest, HandleReloadConfigTest) {
    HttpRequest request;
    HttpResponse response = admin_api_->handleReloadConfig(request);
    
    // Since we're using the real ConfigManager, we can't mock its behavior
    // So we just check that the response is properly formatted
    EXPECT_TRUE(response.status_code == 200 || response.status_code == 500);
    EXPECT_EQ(response.content_type, "application/json");
    
    // Parse the JSON response
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(response.body, root));
    
    if (response.status_code == 200) {
        EXPECT_TRUE(root.isMember("success"));
        EXPECT_TRUE(root["success"].asBool());
        EXPECT_TRUE(root.isMember("message"));
    } else {
        EXPECT_TRUE(root.isMember("error"));
        EXPECT_TRUE(root["error"].asBool());
        EXPECT_TRUE(root.isMember("message"));
    }
}

// Test metrics handler
TEST_F(AdminApiExtendedTest, HandleGetMetricsTest) {
    // Test JSON format
    HttpRequest json_request;
    HttpResponse json_response = admin_api_->handleGetMetrics(json_request);
    
    EXPECT_EQ(json_response.status_code, 200);
    EXPECT_EQ(json_response.content_type, "application/json");
    
    // Parse the JSON response
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(json_response.body, root));
    
    // Test Prometheus format
    HttpRequest prometheus_request;
    prometheus_request.query_params["format"] = "prometheus";
    HttpResponse prometheus_response = admin_api_->handleGetMetrics(prometheus_request);
    
    EXPECT_EQ(prometheus_response.status_code, 200);
    EXPECT_EQ(prometheus_response.content_type, "text/plain; version=0.0.4; charset=utf-8");
}

// Test unimplemented handlers
TEST_F(AdminApiExtendedTest, UnimplementedHandlersTest) {
    HttpRequest request;
    
    // Test all unimplemented handlers
    HttpResponse add_device_response = admin_api_->handleAddDevice(request);
    EXPECT_EQ(add_device_response.status_code, 501);
    
    HttpResponse update_device_response = admin_api_->handleUpdateDevice(request);
    EXPECT_EQ(update_device_response.status_code, 501);
    
    HttpResponse delete_device_response = admin_api_->handleDeleteDevice(request);
    EXPECT_EQ(delete_device_response.status_code, 501);
    
    HttpResponse get_config_response = admin_api_->handleGetConfig(request);
    EXPECT_EQ(get_config_response.status_code, 501);
    
    HttpResponse update_config_response = admin_api_->handleUpdateConfig(request);
    EXPECT_EQ(update_config_response.status_code, 501);
}