#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>

namespace ocpp_gateway {
namespace common {

/**
 * @brief Supported languages
 */
enum class Language {
    ENGLISH,
    JAPANESE
};

/**
 * @brief Exception for i18n-related errors
 */
class I18nError : public std::runtime_error {
public:
    explicit I18nError(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Internationalization Manager for handling multiple languages
 */
class I18nManager {
public:
    /**
     * @brief Construct a new I18n Manager
     */
    I18nManager();

    /**
     * @brief Construct a new I18n Manager with default language
     * 
     * @param default_language Default language
     */
    explicit I18nManager(Language default_language);

    /**
     * @brief Destructor
     */
    ~I18nManager();

    /**
     * @brief Initialize i18n system
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize();

    /**
     * @brief Load language resources from directory
     * 
     * @param resource_dir Path to resource directory
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadResources(const std::string& resource_dir);

    /**
     * @brief Set current language
     * 
     * @param language Language to set
     * @return true if language was set successfully
     * @return false if language setting failed
     */
    bool setLanguage(Language language);

    /**
     * @brief Get current language
     * 
     * @return Language Current language
     */
    Language getCurrentLanguage() const;

    /**
     * @brief Get translated text
     * 
     * @param key Translation key
     * @return std::string Translated text
     */
    std::string getText(const std::string& key) const;

    /**
     * @brief Get translated text with parameters
     * 
     * @param key Translation key
     * @param params Parameters for string formatting
     * @return std::string Translated text with parameters
     */
    std::string getText(const std::string& key, const std::map<std::string, std::string>& params) const;

    /**
     * @brief Get available languages
     * 
     * @return std::vector<Language> List of available languages
     */
    std::vector<Language> getAvailableLanguages() const;

    /**
     * @brief Check if language is available
     * 
     * @param language Language to check
     * @return true if language is available
     * @return false if language is not available
     */
    bool isLanguageAvailable(Language language) const;

    /**
     * @brief Get language name
     * 
     * @param language Language
     * @return std::string Language name
     */
    std::string getLanguageName(Language language) const;

    /**
     * @brief Get language code
     * 
     * @param language Language
     * @return std::string Language code
     */
    std::string getLanguageCode(Language language) const;

    /**
     * @brief Convert language to string
     * 
     * @param language Language
     * @return std::string Language string
     */
    static std::string languageToString(Language language);

    /**
     * @brief Convert string to language
     * 
     * @param language_string Language string
     * @return Language Language
     */
    static Language languageFromString(const std::string& language_string);

    /**
     * @brief Format string with parameters
     * 
     * @param format Format string
     * @param params Parameters
     * @return std::string Formatted string
     */
    static std::string formatString(const std::string& format, const std::map<std::string, std::string>& params);

    /**
     * @brief Get system default language
     * 
     * @return Language System default language
     */
    static Language getSystemDefaultLanguage();

private:
    Language current_language_;
    std::map<Language, std::map<std::string, std::string>> translations_;
    std::vector<Language> available_languages_;

    /**
     * @brief Load language file
     * 
     * @param language Language to load
     * @param file_path Path to language file
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadLanguageFile(Language language, const std::string& file_path);

    /**
     * @brief Initialize default translations
     */
    void initializeDefaultTranslations();

    /**
     * @brief Replace parameters in string
     * 
     * @param text Text with placeholders
     * @param params Parameters
     * @return std::string Text with replaced parameters
     */
    std::string replaceParameters(const std::string& text, const std::map<std::string, std::string>& params) const;
};

} // namespace common
} // namespace ocpp_gateway 