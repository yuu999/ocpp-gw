#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ocpp_gateway/common/language_manager.h"
#include <boost/filesystem.hpp>
#include <fstream>

namespace fs = boost::filesystem;

using namespace ocpp_gateway::common;
using namespace testing;

class LanguageManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary test directory for language resources
        test_resource_dir_ = "test_resources_lang";
        if (fs::exists(test_resource_dir_)) {
            fs::remove_all(test_resource_dir_);
        }
        fs::create_directory(test_resource_dir_);
        
        // Create test language files
        createTestLanguageFiles();
    }

    void TearDown() override {
        // Clean up test directory
        if (fs::exists(test_resource_dir_)) {
            fs::remove_all(test_resource_dir_);
        }
    }
    
    void createTestLanguageFiles() {
        // Create English test file
        std::ofstream en_file(test_resource_dir_ + "/en.json");
        en_file << "{\n";
        en_file << "  \"language\": \"en\",\n";
        en_file << "  \"translations\": {\n";
        en_file << "    \"test_key\": \"Test Value\",\n";
        en_file << "    \"hello\": \"Hello\",\n";
        en_file << "    \"welcome\": \"Welcome to OCPP Gateway\"\n";
        en_file << "  }\n";
        en_file << "}\n";
        en_file.close();
        
        // Create Japanese test file
        std::ofstream ja_file(test_resource_dir_ + "/ja.json");
        ja_file << "{\n";
        ja_file << "  \"language\": \"ja\",\n";
        ja_file << "  \"translations\": {\n";
        ja_file << "    \"test_key\": \"テスト値\",\n";
        ja_file << "    \"hello\": \"こんにちは\",\n";
        ja_file << "    \"welcome\": \"OCPP Gatewayへようこそ\"\n";
        ja_file << "  }\n";
        ja_file << "}\n";
        ja_file.close();
    }

    std::string test_resource_dir_;
};

// Test initialization
TEST_F(LanguageManagerTest, InitializationTest) {
    LanguageManager& manager = LanguageManager::getInstance();
    
    // Initialize with English
    EXPECT_TRUE(manager.initialize("en", test_resource_dir_));
    EXPECT_EQ(manager.getCurrentLanguage(), "en");
    
    // Initialize with Japanese
    EXPECT_TRUE(manager.initialize("ja", test_resource_dir_));
    EXPECT_EQ(manager.getCurrentLanguage(), "ja");
    
    // Initialize with invalid language (should default to English)
    EXPECT_FALSE(manager.initialize("fr", test_resource_dir_));
    EXPECT_EQ(manager.getCurrentLanguage(), "en");
}

// Test language switching
TEST_F(LanguageManagerTest, LanguageSwitchingTest) {
    LanguageManager& manager = LanguageManager::getInstance();
    
    // Initialize with English
    EXPECT_TRUE(manager.initialize("en", test_resource_dir_));
    
    // Switch to Japanese
    EXPECT_TRUE(manager.setLanguage("ja"));
    EXPECT_EQ(manager.getCurrentLanguage(), "ja");
    
    // Switch to invalid language (should fail and keep current language)
    EXPECT_FALSE(manager.setLanguage("fr"));
    EXPECT_EQ(manager.getCurrentLanguage(), "ja");
    
    // Switch back to English
    EXPECT_TRUE(manager.setLanguage("en"));
    EXPECT_EQ(manager.getCurrentLanguage(), "en");
}

// Test translation
TEST_F(LanguageManagerTest, TranslationTest) {
    LanguageManager& manager = LanguageManager::getInstance();
    
    // Initialize with English
    EXPECT_TRUE(manager.initialize("en", test_resource_dir_));
    
    // Test English translations
    EXPECT_EQ(manager.translate("test_key"), "Test Value");
    EXPECT_EQ(manager.translate("hello"), "Hello");
    EXPECT_EQ(manager.translate("welcome"), "Welcome to OCPP Gateway");
    
    // Test missing key with default value
    EXPECT_EQ(manager.translate("missing_key", "Default"), "Default");
    
    // Test missing key without default value (should return key)
    EXPECT_EQ(manager.translate("another_missing_key"), "another_missing_key");
    
    // Switch to Japanese and test translations
    EXPECT_TRUE(manager.setLanguage("ja"));
    EXPECT_EQ(manager.translate("test_key"), "テスト値");
    EXPECT_EQ(manager.translate("hello"), "こんにちは");
    EXPECT_EQ(manager.translate("welcome"), "OCPP Gatewayへようこそ");
}

// Test available languages
TEST_F(LanguageManagerTest, AvailableLanguagesTest) {
    LanguageManager& manager = LanguageManager::getInstance();
    
    // Initialize with English
    EXPECT_TRUE(manager.initialize("en", test_resource_dir_));
    
    // Check available languages
    std::vector<std::string> available_languages = manager.getAvailableLanguages();
    EXPECT_EQ(available_languages.size(), 2);
    EXPECT_TRUE(std::find(available_languages.begin(), available_languages.end(), "en") != available_languages.end());
    EXPECT_TRUE(std::find(available_languages.begin(), available_languages.end(), "ja") != available_languages.end());
}

// Test adding translations
TEST_F(LanguageManagerTest, AddTranslationTest) {
    LanguageManager& manager = LanguageManager::getInstance();
    
    // Initialize with English
    EXPECT_TRUE(manager.initialize("en", test_resource_dir_));
    
    // Add a new translation
    manager.addTranslation("new_key", "New Value");
    EXPECT_EQ(manager.translate("new_key"), "New Value");
    
    // Override an existing translation
    manager.addTranslation("test_key", "Modified Value");
    EXPECT_EQ(manager.translate("test_key"), "Modified Value");
}

// Test loading translations from file
TEST_F(LanguageManagerTest, LoadTranslationsFromFileTest) {
    LanguageManager& manager = LanguageManager::getInstance();
    
    // Initialize with English
    EXPECT_TRUE(manager.initialize("en", test_resource_dir_));
    
    // Create a new language file
    std::ofstream de_file(test_resource_dir_ + "/de.json");
    de_file << "{\n";
    de_file << "  \"language\": \"de\",\n";
    de_file << "  \"translations\": {\n";
    de_file << "    \"test_key\": \"Test Wert\",\n";
    de_file << "    \"hello\": \"Hallo\",\n";
    de_file << "    \"welcome\": \"Willkommen bei OCPP Gateway\"\n";
    de_file << "  }\n";
    de_file << "}\n";
    de_file.close();
    
    // Load the new language file
    EXPECT_TRUE(manager.loadTranslationsFromFile(test_resource_dir_ + "/de.json"));
    
    // Check if the new language is available
    std::vector<std::string> available_languages = manager.getAvailableLanguages();
    EXPECT_TRUE(std::find(available_languages.begin(), available_languages.end(), "de") != available_languages.end());
    
    // Switch to the new language and test translations
    EXPECT_TRUE(manager.setLanguage("de"));
    EXPECT_EQ(manager.translate("test_key"), "Test Wert");
    EXPECT_EQ(manager.translate("hello"), "Hallo");
    EXPECT_EQ(manager.translate("welcome"), "Willkommen bei OCPP Gateway");
}

// Test loading all translations
TEST_F(LanguageManagerTest, LoadAllTranslationsTest) {
    LanguageManager& manager = LanguageManager::getInstance();
    
    // Initialize with empty resource directory
    std::string empty_dir = "empty_resources";
    if (fs::exists(empty_dir)) {
        fs::remove_all(empty_dir);
    }
    fs::create_directory(empty_dir);
    
    // Should fail with empty directory
    EXPECT_FALSE(manager.initialize("en", empty_dir));
    
    // Initialize with test resource directory
    EXPECT_TRUE(manager.initialize("en", test_resource_dir_));
    
    // Add a new language file
    std::ofstream fr_file(test_resource_dir_ + "/fr.json");
    fr_file << "{\n";
    fr_file << "  \"language\": \"fr\",\n";
    fr_file << "  \"translations\": {\n";
    fr_file << "    \"test_key\": \"Valeur de test\",\n";
    fr_file << "    \"hello\": \"Bonjour\",\n";
    fr_file << "    \"welcome\": \"Bienvenue sur OCPP Gateway\"\n";
    fr_file << "  }\n";
    fr_file << "}\n";
    fr_file.close();
    
    // Reload all translations
    EXPECT_TRUE(manager.loadAllTranslations());
    
    // Check if the new language is available
    std::vector<std::string> available_languages = manager.getAvailableLanguages();
    EXPECT_TRUE(std::find(available_languages.begin(), available_languages.end(), "fr") != available_languages.end());
    
    // Clean up empty directory
    fs::remove_all(empty_dir);
}