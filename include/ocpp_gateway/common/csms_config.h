#pragma once

#include <string>
#include <memory>

namespace ocpp_gateway {
namespace config {

/**
 * @brief CSMS (Charging Station Management System) configuration class
 */
class CsmsConfig {
public:
    /**
     * @brief Construct a new CSMS Config object with default values
     */
    CsmsConfig();

    /**
     * @brief Construct a new CSMS Config object from YAML file
     * 
     * @param yaml_file Path to YAML configuration file
     */
    explicit CsmsConfig(const std::string& yaml_file);

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
    const std::string& getUrl() const { return url_; }
    void setUrl(const std::string& url) __attribute__((unused)) { url_ = url; }

    int getReconnectInterval() const { return reconnect_interval_sec_; }
    void setReconnectInterval(int interval) __attribute__((unused)) { reconnect_interval_sec_ = interval; }

    int getMaxReconnectAttempts() const { return max_reconnect_attempts_; }
    void setMaxReconnectAttempts(int attempts) __attribute__((unused)) { max_reconnect_attempts_ = attempts; }

    int getHeartbeatInterval() const __attribute__((unused)) { return heartbeat_interval_sec_; }
    void setHeartbeatInterval(int interval) __attribute__((unused)) { heartbeat_interval_sec_ = interval; }

private:
    std::string url_;
    int reconnect_interval_sec_;
    int max_reconnect_attempts_;
    int heartbeat_interval_sec_;
};

} // namespace config
} // namespace ocpp_gateway