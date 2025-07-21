#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ocpp_gateway/api/cli_manager.h"
#include "ocpp_gateway/common/language_manager.h"
#include <vector>
#include <string>

using namespace ocpp_gateway::api;
using namespace ocpp_gateway::common;
using namespace testing;

class CliManagerI18nTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize language manager with test resources
        LanguageManager::getInstance().initialize("en", "resources/lang");
        cli_manager_ = std::make_unique<CliManager>();
    }

    std::unique_ptr<CliManager> cli_manager_;
};

// Test language command
TEST_F(CliManagerI18nTest, LanguageCommandTest) {
    // Test language command with no arguments
    CliResult language_result = cli_manager_->executeCommand({"language"});
    EXPECT_TRUE(language_result.success);
    EXPECT_FALSE(language_result.output.empty());
    
    // Test language list command
    CliResult language_list_result = cli_manager_->executeCommand({"language", "list"});
    EXPECT_TRUE(language_list_result.success);
    EXPECT_FALSE(language_list_result.output.empty());
    
    // Test language set command with valid language
    CliResult language_set_en_result = cli_manager_->executeCommand({"language", "set", "en"});
    EXPECT_TRUE(language_set_en_result.success);
    EXPECT_EQ(cli_manager_->getCurrentLanguage(), "en");
    
    // Test language set command with valid language
    CliResult language_set_ja_result = cli_manager_->executeCommand({"language", "set", "ja"});
    EXPECT_TRUE(language_set_ja_result.success);
    EXPECT_EQ(cli_manager_->getCurrentLanguage(), "ja");
    
    // Test language set command with invalid language
    CliResult language_set_invalid_result = cli_manager_->executeCommand({"language", "set", "fr"});
    EXPECT_FALSE(language_set_invalid_result.success);
    EXPECT_FALSE(language_set_invalid_result.message.empty());
    
    // Test language set command with missing language
    CliResult language_set_missing_result = cli_manager_->executeCommand({"language", "set"});
    EXPECT_FALSE(language_set_missing_result.success);
    EXPECT_FALSE(language_set_missing_result.message.empty());
    
    // Test language command with invalid subcommand
    CliResult language_invalid_result = cli_manager_->executeCommand({"language", "invalid"});
    EXPECT_FALSE(language_invalid_result.success);
    EXPECT_FALSE(language_invalid_result.message.empty());
}

// Test help command in different languages
TEST_F(CliManagerI18nTest, HelpCommandInDifferentLanguagesTest) {
    // Set language to English
    cli_manager_->setLanguage("en");
    
    // Test help command in English
    CliResult en_help_result = cli_manager_->executeCommand({"help"});
    EXPECT_TRUE(en_help_result.success);
    EXPECT_FALSE(en_help_result.output.empty());
    EXPECT_TRUE(en_help_result.output.find("Available commands") != std::string::npos);
    
    // Set language to Japanese
    cli_manager_->setLanguage("ja");
    
    // Test help command in Japanese
    CliResult ja_help_result = cli_manager_->executeCommand({"help"});
    EXPECT_TRUE(ja_help_result.success);
    EXPECT_FALSE(ja_help_result.output.empty());
    EXPECT_TRUE(ja_help_result.output.find("利用可能なコマンド") != std::string::npos);
}

// Test version command in different languages
TEST_F(CliManagerI18nTest, VersionCommandInDifferentLanguagesTest) {
    // Set language to English
    cli_manager_->setLanguage("en");
    
    // Test version command in English
    CliResult en_version_result = cli_manager_->executeCommand({"version"});
    EXPECT_TRUE(en_version_result.success);
    EXPECT_FALSE(en_version_result.output.empty());
    EXPECT_TRUE(en_version_result.output.find("Version") != std::string::npos);
    
    // Set language to Japanese
    cli_manager_->setLanguage("ja");
    
    // Test version command in Japanese
    CliResult ja_version_result = cli_manager_->executeCommand({"version"});
    EXPECT_TRUE(ja_version_result.success);
    EXPECT_FALSE(ja_version_result.output.empty());
    EXPECT_TRUE(ja_version_result.output.find("バージョン") != std::string::npos);
}

// Test status command in different languages
TEST_F(CliManagerI18nTest, StatusCommandInDifferentLanguagesTest) {
    // Set language to English
    cli_manager_->setLanguage("en");
    
    // Test status command in English
    CliResult en_status_result = cli_manager_->executeCommand({"status"});
    EXPECT_TRUE(en_status_result.success);
    EXPECT_FALSE(en_status_result.output.empty());
    EXPECT_TRUE(en_status_result.output.find("System status") != std::string::npos);
    
    // Set language to Japanese
    cli_manager_->setLanguage("ja");
    
    // Test status command in Japanese
    CliResult ja_status_result = cli_manager_->executeCommand({"status"});
    EXPECT_TRUE(ja_status_result.success);
    EXPECT_FALSE(ja_status_result.output.empty());
    EXPECT_TRUE(ja_status_result.output.find("システム状態") != std::string::npos);
}

// Test error messages in different languages
TEST_F(CliManagerI18nTest, ErrorMessagesInDifferentLanguagesTest) {
    // Set language to English
    cli_manager_->setLanguage("en");
    
    // Test unknown command in English
    CliResult en_unknown_result = cli_manager_->executeCommand({"unknown_command"});
    EXPECT_FALSE(en_unknown_result.success);
    EXPECT_FALSE(en_unknown_result.message.empty());
    EXPECT_TRUE(en_unknown_result.message.find("Unknown command") != std::string::npos);
    
    // Set language to Japanese
    cli_manager_->setLanguage("ja");
    
    // Test unknown command in Japanese
    CliResult ja_unknown_result = cli_manager_->executeCommand({"unknown_command"});
    EXPECT_FALSE(ja_unknown_result.success);
    EXPECT_FALSE(ja_unknown_result.message.empty());
    EXPECT_TRUE(ja_unknown_result.message.find("不明なコマンド") != std::string::npos);
}

// Test translation method
TEST_F(CliManagerI18nTest, TranslationMethodTest) {
    // Set language to English
    cli_manager_->setLanguage("en");
    
    // Test translation with existing key
    EXPECT_EQ(cli_manager_->translate("dashboard", "ダッシュボード"), "Dashboard");
    
    // Test translation with non-existing key
    EXPECT_EQ(cli_manager_->translate("non_existing_key", "デフォルト値"), "デフォルト値");
    
    // Set language to Japanese
    cli_manager_->setLanguage("ja");
    
    // Test translation with existing key
    EXPECT_EQ(cli_manager_->translate("dashboard", "ダッシュボード"), "ダッシュボード");
    
    // Test translation with non-existing key
    EXPECT_EQ(cli_manager_->translate("non_existing_key", "デフォルト値"), "デフォルト値");
}