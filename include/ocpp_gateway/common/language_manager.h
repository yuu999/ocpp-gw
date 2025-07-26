#pragma once

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace ocpp_gateway {
namespace common {

/**
 * @brief Language manager for internationalization support
 * 
 * This class manages language resources and provides translation functionality.
 * It supports loading translations from resource files and switching between languages.
 */
class LanguageManager {
public:
    /**
     * @brief Get the singleton instance of the language manager
     * 
     * @return LanguageManager& The singleton instance
     */
    static LanguageManager& getInstance();

    /**
     * @brief Initialize the language manager with the specified language
     * 
     * @param language The language code (e.g., "en", "ja")
     * @param resource_dir The directory containing language resource files
     * @return bool True if initialization was successful, false otherwise
     */
    bool initialize(const std::string& language, const std::string& resource_dir = "resources/lang");

    /**
     * @brief Set the current language
     * 
     * @param language The language code (e.g., "en", "ja")
     * @return bool True if the language was set successfully, false otherwise
     */
    bool setLanguage(const std::string& language);

    /**
     * @brief Get the current language
     * 
     * @return std::string The current language code
     */
    std::string getCurrentLanguage() const;

    /**
     * @brief Get a list of available languages
     * 
     * @return std::vector<std::string> List of available language codes
     */
    std::vector<std::string> getAvailableLanguages() const;

    /**
     * @brief Get a translated string for the given key
     * 
     * @param key The translation key
     * @param default_value The default value to return if the key is not found
     * @return std::string The translated string or the default value if not found
     */
    std::string translate(const std::string& key, const std::string& default_value = "") const;

    /**
     * @brief Get a translated string for the given key with format arguments
     * 
     * @param key The translation key
     * @param args The format arguments
     * @return std::string The translated and formatted string
     */
    template<typename... Args>
    std::string translateFormat(const std::string& key, Args... args) const;

    /**
     * @brief Add a translation for the given key in the current language
     * 
     * @param key The translation key
     * @param value The translated value
     */
    void addTranslation(const std::string& key, const std::string& value);

    /**
     * @brief Load translations from a file
     * 
     * @param file_path The path to the translation file
     * @param language The language code (optional, uses current language if not specified)
     * @return bool True if the file was loaded successfully, false otherwise
     */
    bool loadTranslationsFromFile(const std::string& file_path, const std::string& language = "");

    /**
     * @brief Load all available translations from the resource directory
     * 
     * @return bool True if at least one language was loaded successfully, false otherwise
     */
    bool loadAllTranslations();

private:
    LanguageManager();
    ~LanguageManager() = default;
    
    // Prevent copying and assignment
    LanguageManager(const LanguageManager&) = delete;
    LanguageManager& operator=(const LanguageManager&) = delete;

    // Create default language resource files (English/Japanese)
    void createDefaultResources();

    std::string current_language_;
    std::string resource_dir_;
    std::map<std::string, std::map<std::string, std::string>> translations_;
    std::vector<std::string> available_languages_;
    mutable std::mutex mutex_;
};

} // namespace common
} // namespace ocpp_gateway 