#pragma once

#include "ocpp_gateway/common/system_config.h"
#include "ocpp_gateway/common/csms_config.h"
#include "ocpp_gateway/common/device_config.h"
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <mutex>

namespace ocpp_gateway {
namespace config {

/**
 * @brief Configuration change callback function type
 */
using ConfigChangeCallback = std::function<void()>;

/**
 * @brief Configuration manager class
 * 
 * This class manages all configuration objects and provides methods to load, save, and validate them.
 * It also supports hot reloading of configurations and notifies registered callbacks when configurations change.
 */
class ConfigManager {
public:
    /**
     * @brief Get the singleton instance of ConfigManager
     * 
     * @return ConfigManager& Singleton instance
     */
    static ConfigManager& getInstance();

    /**
     * @brief Initialize the configuration manager
     * 
     * @param config_dir Directory containing configuration files
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize(const std::string& config_dir);

    /**
     * @brief Load all configurations from files
     * 
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadAllConfigs();

    /**
     * @brief Reload all configurations from files
     * 
     * @return true if reloading was successful
     * @return false if reloading failed
     */
    bool reloadAllConfigs();

    /**
     * @brief Register a callback for configuration changes
     * 
     * @param callback Callback function
     * @return int Callback ID
     */
    int registerChangeCallback(ConfigChangeCallback callback);

    /**
     * @brief Unregister a callback for configuration changes
     * 
     * @param callback_id Callback ID
     * @return true if callback was unregistered
     * @return false if callback was not found
     */
    bool unregisterChangeCallback(int callback_id);

    /**
     * @brief Get the system configuration
     * 
     * @return const SystemConfig& System configuration
     */
    const SystemConfig& getSystemConfig() const;

    /**
     * @brief Get the CSMS configuration
     * 
     * @return const CsmsConfig& CSMS configuration
     */
    const CsmsConfig& getCsmsConfig() const;

    /**
     * @brief Get the device configuration collection
     * 
     * @return const DeviceConfigCollection& Device configuration collection
     */
    const DeviceConfigCollection& getDeviceConfigs() const;

    /**
     * @brief Get a device configuration by ID
     * 
     * @param id Device ID
     * @return std::optional<DeviceConfig> Device configuration if found, empty optional otherwise
     */
    std::optional<DeviceConfig> getDeviceConfig(const std::string& id) const;

    /**
     * @brief Add or update a device configuration
     * 
     * @param device Device configuration
     * @return true if device was added or updated
     * @return false if operation failed
     */
    bool addOrUpdateDeviceConfig(const DeviceConfig& device);

    /**
     * @brief Remove a device configuration by ID
     * 
     * @param id Device ID
     * @return true if device was removed
     * @return false if device was not found
     */
    bool removeDeviceConfig(const std::string& id);

    /**
     * @brief Save all configurations to files
     * 
     * @return true if saving was successful
     * @return false if saving failed
     */
    bool saveAllConfigs();

    /**
     * @brief Validate all configurations
     * 
     * @return true if all configurations are valid
     * @throws ConfigValidationError if any configuration is invalid
     */
    bool validateAllConfigs() const;

private:
    /**
     * @brief Construct a new Config Manager object
     * Private constructor for singleton pattern
     */
    ConfigManager();

    /**
     * @brief Notify all registered callbacks about configuration changes
     */
    void notifyConfigChange();

    /**
     * @brief Start monitoring configuration files for changes
     */
    void startFileMonitoring();

    /**
     * @brief Stop monitoring configuration files for changes
     */
    void stopFileMonitoring();

    /**
     * @brief Handle file change event
     * 
     * @param file_path Path to changed file
     */
    void handleFileChange(const std::string& file_path);

    std::string config_dir_;
    SystemConfig system_config_;
    CsmsConfig csms_config_;
    DeviceConfigCollection device_configs_;

    std::map<int, ConfigChangeCallback> change_callbacks_;
    int next_callback_id_;
    bool file_monitoring_active_;
    std::mutex mutex_;
};

} // namespace config
} // namespace ocpp_gateway