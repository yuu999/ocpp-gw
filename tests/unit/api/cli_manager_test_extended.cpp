#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ocpp_gateway/api/cli_manager.h"
#include "ocpp_gateway/common/config_manager.h"
#include <vector>
#include <string>
#include <memory>

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

class CliManagerExtendedTest : public ::testing::Test {
protected:
    void SetUp() override {
        cli_manager_ = std::make_unique<CliManager>();
    }

    std::unique_ptr<CliManager> cli_manager_;
};

// Test command registration and execution
TEST_F(CliManagerExtendedTest, CommandRegistrationAndExecutionTest) {
    // Register a custom command
    bool command_executed = false;
    cli_manager_->registerCommand("test_command", "Test command description",
        [&command_executed](const std::vector<std::string>& args) {
            command_executed = true;
            return CliResult(true, "Test command executed");
        });
    
    // Execute the custom command
    CliResult result = cli_manager_->executeCommand({"test_command"});
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.message, "Test command executed");
    EXPECT_TRUE(command_executed);
    
    // Execute a non-existent command
    CliResult non_existent_result = cli_manager_->executeCommand({"non_existent_command"});
    EXPECT_FALSE(non_existent_result.success);
    EXPECT_FALSE(non_existent_result.message.empty());
}

// Test command with arguments
TEST_F(CliManagerExtendedTest, CommandWithArgumentsTest) {
    // Register a command that processes arguments
    cli_manager_->registerCommand("echo", "Echo command",
        [](const std::vector<std::string>& args) {
            std::string output;
            for (size_t i = 1; i < args.size(); ++i) {
                if (i > 1) output += " ";
                output += args[i];
            }
            return CliResult(true, "", output);
        });
    
    // Execute the command with arguments
    CliResult result = cli_manager_->executeCommand({"echo", "hello", "world"});
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.output, "hello world");
    
    // Execute the command with no arguments
    CliResult no_args_result = cli_manager_->executeCommand({"echo"});
    EXPECT_TRUE(no_args_result.success);
    EXPECT_EQ(no_args_result.output, "");
}

// Test command that throws an exception
TEST_F(CliManagerExtendedTest, CommandExceptionTest) {
    // Register a command that throws an exception
    cli_manager_->registerCommand("throw", "Command that throws an exception",
        [](const std::vector<std::string>& args) {
            throw std::runtime_error("Test exception");
            return CliResult(true, "This should not be returned");
        });
    
    // Execute the command
    CliResult result = cli_manager_->executeCommand({"throw"});
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.message.find("Test exception") != std::string::npos);
}

// Test help command with custom commands
TEST_F(CliManagerExtendedTest, HelpWithCustomCommandsTest) {
    // Register custom commands
    cli_manager_->registerCommand("custom1", "Custom command 1", [](const std::vector<std::string>& args) { return CliResult(true); });
    cli_manager_->registerCommand("custom2", "Custom command 2", [](const std::vector<std::string>& args) { return CliResult(true); });
    
    // Get help message
    std::string help_message = cli_manager_->getHelpMessage();
    
    // Check if custom commands are included in help
    EXPECT_TRUE(help_message.find("custom1") != std::string::npos);
    EXPECT_TRUE(help_message.find("Custom command 1") != std::string::npos);
    EXPECT_TRUE(help_message.find("custom2") != std::string::npos);
    EXPECT_TRUE(help_message.find("Custom command 2") != std::string::npos);
}

// Test command with subcommands
TEST_F(CliManagerExtendedTest, CommandWithSubcommandsTest) {
    // Register a command with subcommands
    cli_manager_->registerCommand("parent", "Parent command",
        [](const std::vector<std::string>& args) {
            if (args.size() < 2) {
                return CliResult(false, "Missing subcommand");
            }
            
            std::string subcommand = args[1];
            if (subcommand == "sub1") {
                return CliResult(true, "", "Subcommand 1 executed");
            } else if (subcommand == "sub2") {
                return CliResult(true, "", "Subcommand 2 executed");
            } else {
                return CliResult(false, "Unknown subcommand: " + subcommand);
            }
        });
    
    // Execute the command with valid subcommand
    CliResult sub1_result = cli_manager_->executeCommand({"parent", "sub1"});
    EXPECT_TRUE(sub1_result.success);
    EXPECT_EQ(sub1_result.output, "Subcommand 1 executed");
    
    // Execute the command with another valid subcommand
    CliResult sub2_result = cli_manager_->executeCommand({"parent", "sub2"});
    EXPECT_TRUE(sub2_result.success);
    EXPECT_EQ(sub2_result.output, "Subcommand 2 executed");
    
    // Execute the command with invalid subcommand
    CliResult invalid_sub_result = cli_manager_->executeCommand({"parent", "invalid"});
    EXPECT_FALSE(invalid_sub_result.success);
    EXPECT_EQ(invalid_sub_result.message, "Unknown subcommand: invalid");
    
    // Execute the command without subcommand
    CliResult no_sub_result = cli_manager_->executeCommand({"parent"});
    EXPECT_FALSE(no_sub_result.success);
    EXPECT_EQ(no_sub_result.message, "Missing subcommand");
}

// Test command with options
TEST_F(CliManagerExtendedTest, CommandWithOptionsTest) {
    // Register a command that processes options
    cli_manager_->registerCommand("options", "Command with options",
        [](const std::vector<std::string>& args) {
            bool verbose = false;
            bool help = false;
            std::string output_file;
            
            for (size_t i = 1; i < args.size(); ++i) {
                if (args[i] == "--verbose" || args[i] == "-v") {
                    verbose = true;
                } else if (args[i] == "--help" || args[i] == "-h") {
                    help = true;
                } else if ((args[i] == "--output" || args[i] == "-o") && i + 1 < args.size()) {
                    output_file = args[i + 1];
                    ++i;
                }
            }
            
            if (help) {
                return CliResult(true, "", "Help for options command");
            }
            
            std::string result = "Options: verbose=" + std::string(verbose ? "true" : "false");
            if (!output_file.empty()) {
                result += ", output_file=" + output_file;
            }
            
            return CliResult(true, "", result);
        });
    
    // Execute the command with various options
    CliResult verbose_result = cli_manager_->executeCommand({"options", "--verbose"});
    EXPECT_TRUE(verbose_result.success);
    EXPECT_EQ(verbose_result.output, "Options: verbose=true");
    
    CliResult short_verbose_result = cli_manager_->executeCommand({"options", "-v"});
    EXPECT_TRUE(short_verbose_result.success);
    EXPECT_EQ(short_verbose_result.output, "Options: verbose=true");
    
    CliResult output_result = cli_manager_->executeCommand({"options", "--output", "file.txt"});
    EXPECT_TRUE(output_result.success);
    EXPECT_EQ(output_result.output, "Options: verbose=false, output_file=file.txt");
    
    CliResult short_output_result = cli_manager_->executeCommand({"options", "-o", "file.txt"});
    EXPECT_TRUE(short_output_result.success);
    EXPECT_EQ(short_output_result.output, "Options: verbose=false, output_file=file.txt");
    
    CliResult combined_result = cli_manager_->executeCommand({"options", "-v", "-o", "file.txt"});
    EXPECT_TRUE(combined_result.success);
    EXPECT_EQ(combined_result.output, "Options: verbose=true, output_file=file.txt");
    
    CliResult help_result = cli_manager_->executeCommand({"options", "--help"});
    EXPECT_TRUE(help_result.success);
    EXPECT_EQ(help_result.output, "Help for options command");
}

// Test table formatting
TEST_F(CliManagerExtendedTest, TableFormattingTest) {
    // Create a test command that formats a table
    cli_manager_->registerCommand("table", "Command that formats a table",
        [this](const std::vector<std::string>& args) {
            std::vector<std::vector<std::string>> data = {
                {"Row1Col1", "Row1Col2", "Row1Col3"},
                {"Row2Col1", "Row2Col2", "Row2Col3"},
                {"Row3Col1", "Row3Col2", "Row3Col3"}
            };
            
            std::vector<std::string> headers = {"Header1", "Header2", "Header3"};
            
            std::string table = cli_manager_->formatTable(data, headers);
            return CliResult(true, "", table);
        });
    
    // Execute the command
    CliResult result = cli_manager_->executeCommand({"table"});
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.output.empty());
    
    // Check table formatting
    EXPECT_TRUE(result.output.find("Header1") != std::string::npos);
    EXPECT_TRUE(result.output.find("Header2") != std::string::npos);
    EXPECT_TRUE(result.output.find("Header3") != std::string::npos);
    EXPECT_TRUE(result.output.find("Row1Col1") != std::string::npos);
    EXPECT_TRUE(result.output.find("Row2Col2") != std::string::npos);
    EXPECT_TRUE(result.output.find("Row3Col3") != std::string::npos);
    
    // Check for separator line
    EXPECT_TRUE(result.output.find("------") != std::string::npos);
}

// Test empty command
TEST_F(CliManagerExtendedTest, EmptyCommandTest) {
    CliResult result = cli_manager_->executeCommand({});
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.message.empty());
}

// Test command with empty result
TEST_F(CliManagerExtendedTest, EmptyResultTest) {
    // Register a command that returns an empty result
    cli_manager_->registerCommand("empty", "Command with empty result",
        [](const std::vector<std::string>& args) {
            return CliResult();
        });
    
    // Execute the command
    CliResult result = cli_manager_->executeCommand({"empty"});
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.message.empty());
    EXPECT_TRUE(result.output.empty());
}

// Test command with long output
TEST_F(CliManagerExtendedTest, LongOutputTest) {
    // Register a command that returns a long output
    cli_manager_->registerCommand("long", "Command with long output",
        [](const std::vector<std::string>& args) {
            std::string long_output;
            for (int i = 0; i < 1000; ++i) {
                long_output += "Line " + std::to_string(i) + "\n";
            }
            return CliResult(true, "", long_output);
        });
    
    // Execute the command
    CliResult result = cli_manager_->executeCommand({"long"});
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.output.empty());
    EXPECT_TRUE(result.output.length() > 5000);
}

// Test command with special characters
TEST_F(CliManagerExtendedTest, SpecialCharactersTest) {
    // Register a command that handles special characters
    cli_manager_->registerCommand("special", "Command with special characters",
        [](const std::vector<std::string>& args) {
            if (args.size() < 2) {
                return CliResult(true, "", "No special characters provided");
            }
            return CliResult(true, "", "Received: " + args[1]);
        });
    
    // Execute the command with various special characters
    CliResult result1 = cli_manager_->executeCommand({"special", "!@#$%^&*()"});
    EXPECT_TRUE(result1.success);
    EXPECT_EQ(result1.output, "Received: !@#$%^&*()");
    
    CliResult result2 = cli_manager_->executeCommand({"special", "こんにちは"});
    EXPECT_TRUE(result2.success);
    EXPECT_EQ(result2.output, "Received: こんにちは");
    
    CliResult result3 = cli_manager_->executeCommand({"special", "\\n\\t\\r"});
    EXPECT_TRUE(result3.success);
    EXPECT_EQ(result3.output, "Received: \\n\\t\\r");
}

// Test command with quoted arguments
TEST_F(CliManagerExtendedTest, QuotedArgumentsTest) {
    // Register a command that processes quoted arguments
    cli_manager_->registerCommand("quote", "Command with quoted arguments",
        [](const std::vector<std::string>& args) {
            if (args.size() < 2) {
                return CliResult(true, "", "No quoted arguments provided");
            }
            return CliResult(true, "", "Received: " + args[1]);
        });
    
    // Note: In a real CLI, quoted arguments would be handled by the shell
    // Here we're simulating that by passing the quoted string as a single argument
    CliResult result = cli_manager_->executeCommand({"quote", "argument with spaces"});
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.output, "Received: argument with spaces");
}