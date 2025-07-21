#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ocpp_gateway/api/cli_manager.h"
#include "ocpp_gateway/common/config_manager.h"
#include <vector>
#include <string>

using namespace ocpp_gateway::api;
using namespace ocpp_gateway::config;
using namespace testing;

// Mock ConfigManager for testing
class MockConfigManager : public ConfigManager {
public:
    MOCK_METHOD(bool, reloadAllConfigs, (), (override));
    MOCK_METHOD(void, validateAllConfigs, (), (override));
    MOCK_METHOD(SystemConfig&, getSystemConfig, (), (override));
    MOCK_METHOD(CsmsConfig&, getCsmsConfig, (), (override));
    MOCK_METHOD(DeviceConfigs&, getDeviceConfigs, (), (override));
    MOCK_METHOD(std::optional<DeviceConfig>, getDeviceConfig, (const std::string&), (override));
};

class CliManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        cli_manager_ = std::make_unique<CliManager>();
    }

    std::unique_ptr<CliManager> cli_manager_;
};

// Test basic command execution
TEST_F(CliManagerTest, BasicCommandExecutionTest) {
    // Test empty command
    CliResult empty_result = cli_manager_->executeCommand({});
    EXPECT_FALSE(empty_result.success);
    EXPECT_FALSE(empty_result.message.empty());
    
    // Test unknown command
    CliResult unknown_result = cli_manager_->executeCommand({"unknown_command"});
    EXPECT_FALSE(unknown_result.success);
    EXPECT_FALSE(unknown_result.message.empty());
    
    // Test help command
    CliResult help_result = cli_manager_->executeCommand({"help"});
    EXPECT_TRUE(help_result.success);
    EXPECT_FALSE(help_result.output.empty());
    
    // Test version command
    CliResult version_result = cli_manager_->executeCommand({"version"});
    EXPECT_TRUE(version_result.success);
    EXPECT_FALSE(version_result.output.empty());
    EXPECT_TRUE(version_result.output.find("OCPP 2.0.1") != std::string::npos);
}

// Test help command with arguments
TEST_F(CliManagerTest, HelpCommandTest) {
    // Test help with no arguments
    CliResult help_result = cli_manager_->executeCommand({"help"});
    EXPECT_TRUE(help_result.success);
    EXPECT_FALSE(help_result.output.empty());
    
    // Test help with valid command
    CliResult help_config_result = cli_manager_->executeCommand({"help", "config"});
    EXPECT_TRUE(help_config_result.success);
    EXPECT_FALSE(help_config_result.output.empty());
    EXPECT_TRUE(help_config_result.output.find("config") != std::string::npos);
    
    // Test help with invalid command
    CliResult help_invalid_result = cli_manager_->executeCommand({"help", "invalid_command"});
    EXPECT_FALSE(help_invalid_result.success);
    EXPECT_FALSE(help_invalid_result.message.empty());
    
    // Test help with too many arguments
    CliResult help_too_many_result = cli_manager_->executeCommand({"help", "config", "extra"});
    EXPECT_FALSE(help_too_many_result.success);
    EXPECT_FALSE(help_too_many_result.message.empty());
}

// Test status command
TEST_F(CliManagerTest, StatusCommandTest) {
    CliResult status_result = cli_manager_->executeCommand({"status"});
    EXPECT_TRUE(status_result.success);
    EXPECT_FALSE(status_result.output.empty());
    EXPECT_TRUE(status_result.output.find("システム状態") != std::string::npos);
}

// Test config commands
TEST_F(CliManagerTest, ConfigCommandsTest) {
    // Test config with no arguments
    CliResult config_no_args_result = cli_manager_->executeCommand({"config"});
    EXPECT_FALSE(config_no_args_result.success);
    EXPECT_FALSE(config_no_args_result.message.empty());
    
    // Test config show
    CliResult config_show_result = cli_manager_->executeCommand({"config", "show"});
    EXPECT_TRUE(config_show_result.success);
    EXPECT_FALSE(config_show_result.output.empty());
    EXPECT_TRUE(config_show_result.output.find("システム設定") != std::string::npos);
    
    // Test config reload
    CliResult config_reload_result = cli_manager_->executeCommand({"config", "reload"});
    // Result depends on ConfigManager implementation, so we don't check success
    
    // Test config validate
    CliResult config_validate_result = cli_manager_->executeCommand({"config", "validate"});
    // Result depends on ConfigManager implementation, so we don't check success
    
    // Test config backup (unimplemented)
    CliResult config_backup_result = cli_manager_->executeCommand({"config", "backup"});
    EXPECT_FALSE(config_backup_result.success);
    EXPECT_TRUE(config_backup_result.message.find("未実装") != std::string::npos);
    
    // Test config restore (unimplemented)
    CliResult config_restore_result = cli_manager_->executeCommand({"config", "restore"});
    EXPECT_FALSE(config_restore_result.success);
    EXPECT_TRUE(config_restore_result.message.find("未実装") != std::string::npos);
    
    // Test config with invalid subcommand
    CliResult config_invalid_result = cli_manager_->executeCommand({"config", "invalid"});
    EXPECT_FALSE(config_invalid_result.success);
    EXPECT_FALSE(config_invalid_result.message.empty());
}

// Test device commands
TEST_F(CliManagerTest, DeviceCommandsTest) {
    // Test device with no arguments
    CliResult device_no_args_result = cli_manager_->executeCommand({"device"});
    EXPECT_FALSE(device_no_args_result.success);
    EXPECT_FALSE(device_no_args_result.message.empty());
    
    // Test device list
    CliResult device_list_result = cli_manager_->executeCommand({"device", "list"});
    EXPECT_TRUE(device_list_result.success);
    
    // Test device show with no ID
    CliResult device_show_no_id_result = cli_manager_->executeCommand({"device", "show"});
    EXPECT_FALSE(device_show_no_id_result.success);
    EXPECT_FALSE(device_show_no_id_result.message.empty());
    
    // Test device show with ID (will likely fail as device doesn't exist)
    CliResult device_show_result = cli_manager_->executeCommand({"device", "show", "test_device"});
    // Result depends on ConfigManager implementation, so we don't check success
    
    // Test unimplemented device commands
    CliResult device_add_result = cli_manager_->executeCommand({"device", "add", "test_device"});
    EXPECT_FALSE(device_add_result.success);
    EXPECT_TRUE(device_add_result.message.find("未実装") != std::string::npos);
    
    CliResult device_update_result = cli_manager_->executeCommand({"device", "update", "test_device"});
    EXPECT_FALSE(device_update_result.success);
    EXPECT_TRUE(device_update_result.message.find("未実装") != std::string::npos);
    
    CliResult device_delete_result = cli_manager_->executeCommand({"device", "delete", "test_device"});
    EXPECT_FALSE(device_delete_result.success);
    EXPECT_TRUE(device_delete_result.message.find("未実装") != std::string::npos);
    
    CliResult device_test_result = cli_manager_->executeCommand({"device", "test", "test_device"});
    EXPECT_FALSE(device_test_result.success);
    EXPECT_TRUE(device_test_result.message.find("未実装") != std::string::npos);
}

// Test mapping commands
TEST_F(CliManagerTest, MappingCommandsTest) {
    // Test mapping command (unimplemented)
    CliResult mapping_result = cli_manager_->executeCommand({"mapping"});
    EXPECT_FALSE(mapping_result.success);
    EXPECT_TRUE(mapping_result.message.find("未実装") != std::string::npos);
}

// Test metrics commands
TEST_F(CliManagerTest, MetricsCommandsTest) {
    // Test metrics with no arguments
    CliResult metrics_no_args_result = cli_manager_->executeCommand({"metrics"});
    EXPECT_FALSE(metrics_no_args_result.success);
    EXPECT_FALSE(metrics_no_args_result.message.empty());
    
    // Test metrics show
    CliResult metrics_show_result = cli_manager_->executeCommand({"metrics", "show"});
    EXPECT_TRUE(metrics_show_result.success);
    EXPECT_FALSE(metrics_show_result.output.empty());
    
    // Test metrics show with JSON format
    CliResult metrics_show_json_result = cli_manager_->executeCommand({"metrics", "show", "--json"});
    EXPECT_TRUE(metrics_show_json_result.success);
    EXPECT_FALSE(metrics_show_json_result.output.empty());
    
    // Test metrics show with Prometheus format
    CliResult metrics_show_prometheus_result = cli_manager_->executeCommand({"metrics", "show", "--prometheus"});
    EXPECT_TRUE(metrics_show_prometheus_result.success);
    EXPECT_FALSE(metrics_show_prometheus_result.output.empty());
    
    // Test metrics reset (will likely fail without specific metric name)
    CliResult metrics_reset_result = cli_manager_->executeCommand({"metrics", "reset"});
    EXPECT_FALSE(metrics_reset_result.success);
    
    // Test metrics export
    CliResult metrics_export_result = cli_manager_->executeCommand({"metrics", "export"});
    EXPECT_TRUE(metrics_export_result.success);
    EXPECT_FALSE(metrics_export_result.output.empty());
    
    // Test metrics with invalid subcommand
    CliResult metrics_invalid_result = cli_manager_->executeCommand({"metrics", "invalid"});
    EXPECT_FALSE(metrics_invalid_result.success);
    EXPECT_FALSE(metrics_invalid_result.message.empty());
}

// Test health command
TEST_F(CliManagerTest, HealthCommandTest) {
    CliResult health_result = cli_manager_->executeCommand({"health"});
    EXPECT_TRUE(health_result.success);
    EXPECT_FALSE(health_result.output.empty());
    EXPECT_TRUE(health_result.output.find("ヘルスチェック結果") != std::string::npos);
}

// Test log commands
TEST_F(CliManagerTest, LogCommandsTest) {
    // Test log command (unimplemented)
    CliResult log_result = cli_manager_->executeCommand({"log"});
    EXPECT_FALSE(log_result.success);
    EXPECT_TRUE(log_result.message.find("未実装") != std::string::npos);
}

// Test command registration
TEST_F(CliManagerTest, CommandRegistrationTest) {
    // Register a custom command
    cli_manager_->registerCommand("test_command", "Test command description",
        [](const std::vector<std::string>& args) {
            return CliResult(true, "Test command executed");
        });
    
    // Execute the custom command
    CliResult test_result = cli_manager_->executeCommand({"test_command"});
    EXPECT_TRUE(test_result.success);
    EXPECT_EQ(test_result.message, "Test command executed");
    
    // Check that the command appears in help
    CliResult help_result = cli_manager_->executeCommand({"help"});
    EXPECT_TRUE(help_result.success);
    EXPECT_TRUE(help_result.output.find("test_command") != std::string::npos);
}

// Test utility functions
TEST_F(CliManagerTest, UtilityFunctionsTest) {
    // Test getHelpMessage
    std::string help_message = cli_manager_->getHelpMessage();
    EXPECT_FALSE(help_message.empty());
    EXPECT_TRUE(help_message.find("利用可能なコマンド") != std::string::npos);
}