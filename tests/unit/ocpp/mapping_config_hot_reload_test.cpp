#include "ocpp_gateway/ocpp/mapping_config.h"
#include "ocpp_gateway/common/logger.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>

namespace fs = std::filesystem;

class MappingConfigHotReloadTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger
        ocpp_gateway::common::LogConfig log_config;
        log_config.console_output = true;
        log_config.file_output = false;
        log_config.log_level = "debug";
        ocpp_gateway::common::Logger::initialize(log_config);

        // Create test directory
        test_dir_ = fs::temp_directory_path() / "mapping_config_hot_reload_test";
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        // Remove test directory
        fs::remove_all(test_dir_);
    }

    // Helper method to create a test template file
    void createTemplateFile(const std::string& file_name, const std::string& template_id, 
                           const std::string& description) {
        std::string yaml_content = "template:\n"
                                  "  id: " + template_id + "\n"
                                  "  description: " + description + "\n"
                                  "  variables:\n"
                                  "    - ocpp_name: AvailabilityState\n"
                                  "      type: modbus\n"
                                  "      register: 40001\n"
                                  "      data_type: uint16\n"
                                  "      enum:\n"
                                  "        0: Available\n"
                                  "        1: Occupied\n";

        fs::path file_path = test_dir_ / file_name;
        std::ofstream file(file_path);
        file << yaml_content;
        file.close();
    }

    // Helper method to modify a test template file
    void modifyTemplateFile(const std::string& file_name, const std::string& template_id, 
                           const std::string& description) {
        std::string yaml_content = "template:\n"
                                  "  id: " + template_id + "\n"
                                  "  description: " + description + " (Modified)\n"
                                  "  variables:\n"
                                  "    - ocpp_name: AvailabilityState\n"
                                  "      type: modbus\n"
                                  "      register: 40002\n"  // Changed register
                                  "      data_type: uint16\n"
                                  "      enum:\n"
                                  "        0: Available\n"
                                  "        1: Occupied\n"
                                  "    - ocpp_name: MeterValue\n"  // Added variable
                                  "      type: modbus\n"
                                  "      register: 40010\n"
                                  "      data_type: float32\n"
                                  "      scale: 0.1\n"
                                  "      unit: kWh\n";

        fs::path file_path = test_dir_ / file_name;
        std::ofstream file(file_path);
        file << yaml_content;
        file.close();
    }

    // Helper method to create an invalid template file
    void createInvalidTemplateFile(const std::string& file_name) {
        std::string yaml_content = "template:\n"
                                  "  id: invalid_template\n"
                                  "  description: Invalid Template\n"
                                  "  variables:\n"
                                  "    - ocpp_name: AvailabilityState\n"
                                  "      type: modbus\n"
                                  "      register: -1\n"  // Invalid register
                                  "      data_type: uint16\n";

        fs::path file_path = test_dir_ / file_name;
        std::ofstream file(file_path);
        file << yaml_content;
        file.close();
    }

    fs::path test_dir_;
};

TEST_F(MappingConfigHotReloadTest, TestEnableDisableHotReload) {
    ocpp_gateway::ocpp::MappingTemplateCollection collection;
    
    // Test enabling hot reload
    EXPECT_TRUE(collection.enableHotReload(test_dir_.string()));
    EXPECT_TRUE(collection.isHotReloadEnabled());
    EXPECT_EQ(collection.getWatchedDirectory(), test_dir_.string());
    
    // Test disabling hot reload
    collection.disableHotReload();
    EXPECT_FALSE(collection.isHotReloadEnabled());
    EXPECT_TRUE(collection.getWatchedDirectory().empty());
}

TEST_F(MappingConfigHotReloadTest, TestHotReloadCallback) {
    ocpp_gateway::ocpp::MappingTemplateCollection collection;
    
    // Create initial template file
    createTemplateFile("template1.yaml", "template1", "Test Template 1");
    
    // Load initial template
    EXPECT_TRUE(collection.loadFromDirectory(test_dir_.string()));
    EXPECT_EQ(collection.getTemplates().size(), 1);
    
    // Set up callback flag
    std::atomic<bool> callback_called(false);
    std::string callback_file_path;
    
    // Enable hot reload with callback
    EXPECT_TRUE(collection.enableHotReload(test_dir_.string(), 
        [&callback_called, &callback_file_path](const std::string& file_path) {
            callback_called = true;
            callback_file_path = file_path;
        }
    ));
    
    // Wait a bit to ensure file watcher is running
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Modify the template file
    modifyTemplateFile("template1.yaml", "template1", "Test Template 1");
    
    // Wait for file watcher to detect the change
    // This might need adjustment based on polling interval
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Check if callback was called
    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(callback_file_path.empty());
    
    // Check if template was updated
    auto template_opt = collection.getTemplate("template1");
    ASSERT_TRUE(template_opt.has_value());
    EXPECT_EQ(template_opt->getDescription(), "Test Template 1 (Modified)");
    EXPECT_EQ(template_opt->getVariables().size(), 2);
    
    // Check if the register was updated
    auto var = template_opt->getVariable("AvailabilityState");
    ASSERT_TRUE(var.has_value());
    ASSERT_TRUE(std::holds_alternative<ocpp_gateway::ocpp::ModbusVariableMapping>(var->mapping));
    EXPECT_EQ(std::get<ocpp_gateway::ocpp::ModbusVariableMapping>(var->mapping).register_address, 40002);
    
    // Check if the new variable was added
    var = template_opt->getVariable("MeterValue");
    ASSERT_TRUE(var.has_value());
}

TEST_F(MappingConfigHotReloadTest, TestHotReloadInvalidFile) {
    ocpp_gateway::ocpp::MappingTemplateCollection collection;
    
    // Create initial template file
    createTemplateFile("template1.yaml", "template1", "Test Template 1");
    
    // Load initial template
    EXPECT_TRUE(collection.loadFromDirectory(test_dir_.string()));
    EXPECT_EQ(collection.getTemplates().size(), 1);
    
    // Get initial template for comparison
    auto initial_template = collection.getTemplate("template1");
    ASSERT_TRUE(initial_template.has_value());
    
    // Enable hot reload
    EXPECT_TRUE(collection.enableHotReload(test_dir_.string()));
    
    // Wait a bit to ensure file watcher is running
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Create an invalid template file
    createInvalidTemplateFile("invalid_template.yaml");
    
    // Wait for file watcher to detect the change
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Check that the invalid template was not added
    EXPECT_EQ(collection.getTemplates().size(), 1);
    EXPECT_FALSE(collection.getTemplate("invalid_template").has_value());
    
    // Check that the original template is still intact
    auto template_opt = collection.getTemplate("template1");
    ASSERT_TRUE(template_opt.has_value());
    EXPECT_EQ(template_opt->getDescription(), initial_template->getDescription());
}

TEST_F(MappingConfigHotReloadTest, TestMultipleCallbacks) {
    ocpp_gateway::ocpp::MappingTemplateCollection collection;
    
    // Create initial template file
    createTemplateFile("template1.yaml", "template1", "Test Template 1");
    
    // Load initial template
    EXPECT_TRUE(collection.loadFromDirectory(test_dir_.string()));
    
    // Set up callback flags
    std::atomic<int> callback_count(0);
    
    // Enable hot reload
    EXPECT_TRUE(collection.enableHotReload(test_dir_.string()));
    
    // Register multiple callbacks
    collection.registerChangeCallback([&callback_count](const std::string&) {
        callback_count++;
    });
    
    collection.registerChangeCallback([&callback_count](const std::string&) {
        callback_count++;
    });
    
    // Wait a bit to ensure file watcher is running
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Modify the template file
    modifyTemplateFile("template1.yaml", "template1", "Test Template 1");
    
    // Wait for file watcher to detect the change
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Check if both callbacks were called
    EXPECT_EQ(callback_count, 2);
    
    // Clear callbacks
    collection.clearChangeCallbacks();
    
    // Reset counter
    callback_count = 0;
    
    // Modify the template file again
    modifyTemplateFile("template1.yaml", "template1", "Test Template 1 Updated");
    
    // Wait for file watcher to detect the change
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Check that no callbacks were called
    EXPECT_EQ(callback_count, 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}