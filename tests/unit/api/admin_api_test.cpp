#include <gtest/gtest.h>
#include "ocpp_gateway/api/admin_api.h"
#include "ocpp_gateway/common/config_manager.h"
#include <thread>
#include <chrono>

using namespace ocpp_gateway::api;
using namespace ocpp_gateway::config;

class AdminApiTest : public ::testing::Test {
protected:
    void SetUp() override {
        // テスト用のポート番号を使用（既存のサーバーと競合を避ける）
        admin_api_ = std::make_unique<AdminApi>(9999, "127.0.0.1");
    }

    void TearDown() override {
        if (admin_api_) {
            admin_api_->stop();
        }
    }

    std::unique_ptr<AdminApi> admin_api_;
};

TEST_F(AdminApiTest, ConstructorTest) {
    EXPECT_NE(admin_api_, nullptr);
    EXPECT_FALSE(admin_api_->isRunning());
}

TEST_F(AdminApiTest, StartStopTest) {
    // テスト環境では実際のネットワークバインドをスキップ
    // 基本的なライフサイクルのテスト
    EXPECT_FALSE(admin_api_->isRunning());
    
    // 実際の起動テストはintegrationテストで行う
    // ここでは状態管理のみテスト
    admin_api_->stop(); // 停止操作は安全であることを確認
    EXPECT_FALSE(admin_api_->isRunning());
}

TEST_F(AdminApiTest, AuthenticationTest) {
    // 認証設定のテスト
    admin_api_->setAuthentication(true, "test_user", "test_pass");
    
    // 内部状態は直接テストできないが、例外が発生しないことを確認
    EXPECT_NO_THROW(admin_api_->setAuthentication(false));
}

TEST_F(AdminApiTest, ErrorResponseTest) {
    // エラーレスポンス生成のテスト（プライベートメソッドは間接的にテスト）
    EXPECT_NO_THROW(auto response = admin_api_->createErrorResponse(404, "Test Error"));
}

// 簡易的なHTTPレスポンス構造体の検証
TEST(AdminApiHttpTest, HttpResponseStructTest) {
    HttpResponse response;
    response.status_code = 200;
    response.content_type = "application/json";
    response.body = "{\"test\": true}";
    response.headers["Cache-Control"] = "no-cache";
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "application/json");
    EXPECT_EQ(response.body, "{\"test\": true}");
    EXPECT_EQ(response.headers["Cache-Control"], "no-cache");
}

TEST(AdminApiHttpTest, HttpRequestStructTest) {
    HttpRequest request;
    request.method = HttpMethod::GET;
    request.path = "/api/v1/test";
    request.body = "";
    request.headers["Accept"] = "application/json";
    request.query_params["format"] = "json";
    
    EXPECT_EQ(request.method, HttpMethod::GET);
    EXPECT_EQ(request.path, "/api/v1/test");
    EXPECT_EQ(request.headers["Accept"], "application/json");
    EXPECT_EQ(request.query_params["format"], "json");
} 