#pragma once

#include "ocpp_gateway/common/config_types.h"
#include <string>
#include <memory>

namespace ocpp_gateway {
namespace config {

/**
 * @brief System configuration class
 */
class SystemConfig {
public:
    /**
     * @brief Construct a new System Config object with default values
     */
    SystemConfig();

    /**
     * @brief Construct a new System Config object from YAML file
     * 
     * @param yaml_file Path to YAML configuration file
     */
    explicit SystemConfig(const std::string& yaml_file);

    /**
     * @brief Load configuration from YAML file
     * 
     * @param yaml_file Path to YAML configuration file
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromYaml(const std::string& yaml_file);

    /**
     * @brief Load configuration from YAML string
     * 
     * @param yaml_content YAML content as string
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromYamlString(const std::string& yaml_content);

    /**
     * @brief Load configuration from JSON file
     * 
     * @param json_file Path to JSON configuration file
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromJson(const std::string& json_file);

    /**
     * @brief Load configuration from JSON string
     * 
     * @param json_content JSON content as string
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromJsonString(const std::string& json_content);

    /**
     * @brief Save configuration to YAML file
     * 
     * @param yaml_file Path to YAML configuration file
     * @return true if saving was successful
     * @return false if saving failed
     */
    bool saveToYaml(const std::string& yaml_file) const;

    /**
     * @brief Save configuration to JSON file
     * 
     * @param json_file Path to JSON configuration file
     * @return true if saving was successful
     * @return false if saving failed
     */
    bool saveToJson(const std::string& json_file) const;

    /**
     * @brief Validate the configuration
     * 
     * @return true if configuration is valid
     * @throws ConfigValidationError if configuration is invalid
     */
    bool validate() const;

    // Getters and setters
    LogLevel getLogLevel() const { return log_level_; }
    void setLogLevel(LogLevel level) __attribute__((unused)) { log_level_ = level; }

    const LogRotationConfig& getLogRotation() const __attribute__((unused)) { return log_rotation_; }
    void setLogRotation(const LogRotationConfig& config) __attribute__((unused)) { log_rotation_ = config; }

    const MetricsConfig& getMetrics() const __attribute__((unused)) { return metrics_; }
    void setMetrics(const MetricsConfig& config) __attribute__((unused)) { metrics_ = config; }

    const SecurityConfig& getSecurity() const __attribute__((unused)) { return security_; }
    void setSecurity(const SecurityConfig& config) __attribute__((unused)) { security_ = config; }

private:
    LogLevel log_level_;
    LogRotationConfig log_rotation_;
    MetricsConfig metrics_;
    SecurityConfig security_;
};

} // namespace config
} // namespace ocpp_gateway