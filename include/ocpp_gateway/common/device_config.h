#pragma once

#include "ocpp_gateway/common/config_types.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace ocpp_gateway {
namespace config {

/**
 * @brief Device configuration class
 */
class DeviceConfig {
public:
    /**
     * @brief Construct a new Device Config object with default values
     */
    DeviceConfig();

    /**
     * @brief Construct a new Device Config object with specified values
     * 
     * @param id Device ID
     * @param template_id Template ID
     * @param protocol Protocol type
     * @param connection Connection configuration
     * @param ocpp_id OCPP ID
     */
    DeviceConfig(
        const std::string& id,
        const std::string& template_id,
        ProtocolType protocol,
        const ConnectionConfig& connection,
        const std::string& ocpp_id
    );

    /**
     * @brief Load device configuration from YAML file
     * 
     * @param yaml_file Path to YAML configuration file
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromYaml(const std::string& yaml_file);

    /**
     * @brief Load device configuration from YAML string
     * 
     * @param yaml_content YAML content as string
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromYamlString(const std::string& yaml_content);

    /**
     * @brief Load device configuration from JSON file
     * 
     * @param json_file Path to JSON configuration file
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromJson(const std::string& json_file);

    /**
     * @brief Load device configuration from JSON string
     * 
     * @param json_content JSON content as string
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromJsonString(const std::string& json_content);

    /**
     * @brief Save device configuration to YAML file
     * 
     * @param yaml_file Path to YAML configuration file
     * @return true if saving was successful
     * @return false if saving failed
     */
    bool saveToYaml(const std::string& yaml_file) const;

    /**
     * @brief Save device configuration to JSON file
     * 
     * @param json_file Path to JSON configuration file
     * @return true if saving was successful
     * @return false if saving failed
     */
    bool saveToJson(const std::string& json_file) const;

    /**
     * @brief Validate the device configuration
     * 
     * @return true if configuration is valid
     * @throws ConfigValidationError if configuration is invalid
     */
    bool validate() const;

    // Getters and setters
    const std::string& getId() const { return id_; }
    void setId(const std::string& id) { id_ = id; }

    const std::string& getTemplateId() const { return template_id_; }
    void setTemplateId(const std::string& template_id) { template_id_ = template_id; }

    ProtocolType getProtocol() const { return protocol_; }
    void setProtocol(ProtocolType protocol) { protocol_ = protocol; }

    const ConnectionConfig& getConnection() const { return connection_; }
    void setConnection(const ConnectionConfig& connection) { connection_ = connection; }

    const std::string& getOcppId() const { return ocpp_id_; }
    void setOcppId(const std::string& ocpp_id) { ocpp_id_ = ocpp_id; }

private:
    std::string id_;
    std::string template_id_;
    ProtocolType protocol_;
    ConnectionConfig connection_;
    std::string ocpp_id_;
};

/**
 * @brief Device configuration collection class
 */
class DeviceConfigCollection {
public:
    /**
     * @brief Construct a new Device Config Collection object
     */
    DeviceConfigCollection();

    /**
     * @brief Construct a new Device Config Collection object from YAML file
     * 
     * @param yaml_file Path to YAML configuration file
     */
    explicit DeviceConfigCollection(const std::string& yaml_file);

    /**
     * @brief Load device configurations from YAML file
     * 
     * @param yaml_file Path to YAML configuration file
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromYaml(const std::string& yaml_file);

    /**
     * @brief Load device configurations from YAML string
     * 
     * @param yaml_content YAML content as string
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromYamlString(const std::string& yaml_content);

    /**
     * @brief Load device configurations from JSON file
     * 
     * @param json_file Path to JSON configuration file
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromJson(const std::string& json_file);

    /**
     * @brief Load device configurations from JSON string
     * 
     * @param json_content JSON content as string
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromJsonString(const std::string& json_content);

    /**
     * @brief Save device configurations to YAML file
     * 
     * @param yaml_file Path to YAML configuration file
     * @return true if saving was successful
     * @return false if saving failed
     */
    bool saveToYaml(const std::string& yaml_file) const;

    /**
     * @brief Save device configurations to JSON file
     * 
     * @param json_file Path to JSON configuration file
     * @return true if saving was successful
     * @return false if saving failed
     */
    bool saveToJson(const std::string& json_file) const;

    /**
     * @brief Validate all device configurations
     * 
     * @return true if all configurations are valid
     * @throws ConfigValidationError if any configuration is invalid
     */
    bool validate() const;

    /**
     * @brief Add a device configuration
     * 
     * @param device Device configuration
     */
    void addDevice(const DeviceConfig& device);

    /**
     * @brief Remove a device configuration by ID
     * 
     * @param id Device ID
     * @return true if device was removed
     * @return false if device was not found
     */
    bool removeDevice(const std::string& id);

    /**
     * @brief Get a device configuration by ID
     * 
     * @param id Device ID
     * @return std::optional<DeviceConfig> Device configuration if found, empty optional otherwise
     */
    std::optional<DeviceConfig> getDevice(const std::string& id) const;

    /**
     * @brief Get all device configurations
     * 
     * @return const std::vector<DeviceConfig>& Vector of device configurations
     */
    const std::vector<DeviceConfig>& getDevices() const { return devices_; }

private:
    std::vector<DeviceConfig> devices_;
};

} // namespace config
} // namespace ocpp_gateway