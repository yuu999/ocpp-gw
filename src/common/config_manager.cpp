#include "ocpp_gateway/common/config_manager.h"
#include "ocpp_gateway/common/logger.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace ocpp_gateway {
namespace config {

namespace fs = std::filesystem;

// Singleton instance
ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager()
    : config_dir_(""),
      next_callback_id_(0),
      file_monitoring_active_(false) {
}

bool ConfigManager::initialize(const std::string& config_dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    config_dir_ = config_dir;
    
    // Ensure config directory exists
    if (!fs::exists(config_dir_)) {
        try {
            fs::create_directories(config_dir_);
            LOG_INFO("設定ディレクトリを作成しました: {}", config_dir_);
        } catch (const std::exception& e) {
            LOG_ERROR("設定ディレクトリの作成に失敗しました: {} - {}", config_dir_, e.what());
            return false;
        }
    }
    
    // Load all configurations
    if (!loadAllConfigs()) {
        LOG_ERROR("設定の読み込みに失敗しました");
        return false;
    }
    
    LOG_INFO("設定マネージャーを初期化しました: {}", config_dir_);
    
    // Start file monitoring
    startFileMonitoring();
    
    return true;
}

bool ConfigManager::loadAllConfigs() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Load system configuration
    std::string system_config_path = config_dir_ + "/system.yaml";
    if (fs::exists(system_config_path)) {
        if (!system_config_.loadFromYaml(system_config_path)) {
            LOG_ERROR("システム設定の読み込みに失敗しました: {}", system_config_path);
            return false;
        }
        LOG_DEBUG("システム設定を読み込みました: {}", system_config_path);
    } else {
        LOG_WARN("システム設定ファイルが見つかりません: {}", system_config_path);
    }
    
    // Load CSMS configuration
    std::string csms_config_path = config_dir_ + "/csms.yaml";
    if (fs::exists(csms_config_path)) {
        if (!csms_config_.loadFromYaml(csms_config_path)) {
            LOG_ERROR("CSMS設定の読み込みに失敗しました: {}", csms_config_path);
            return false;
        }
        LOG_DEBUG("CSMS設定を読み込みました: {}", csms_config_path);
    } else {
        LOG_WARN("CSMS設定ファイルが見つかりません: {}", csms_config_path);
    }
    
    // Load device configurations
    std::string devices_dir = config_dir_ + "/devices";
    if (fs::exists(devices_dir)) {
        for (const auto& entry : fs::directory_iterator(devices_dir)) {
            if (entry.is_regular_file()) {
                std::string file_path = entry.path().string();
                if (boost::algorithm::ends_with(file_path, ".yaml") || boost::algorithm::ends_with(file_path, ".yml")) {
                    DeviceConfigCollection device_collection;
                    if (device_collection.loadFromYaml(file_path)) {
                        for (const auto& device : device_collection.getDevices()) {
                            device_configs_.addDevice(device);
                        }
                    }
                }
            }
        }
    }
    
    return true;
}

bool ConfigManager::reloadAllConfigs() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create backup of current configurations
    SystemConfig backup_system_config = system_config_;
    CsmsConfig backup_csms_config = csms_config_;
    DeviceConfigCollection backup_device_configs = device_configs_;
    
    // Clear device configurations
    device_configs_ = DeviceConfigCollection();
    
    // Try to load all configurations
    if (!loadAllConfigs()) {
        // Restore backup if loading fails
        system_config_ = backup_system_config;
        csms_config_ = backup_csms_config;
        device_configs_ = backup_device_configs;
        return false;
    }
    
    // Notify all callbacks about configuration changes
    notifyConfigChange();
    
    return true;
}

int ConfigManager::registerChangeCallback(ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int callback_id = next_callback_id_++;
    change_callbacks_[callback_id] = callback;
    
    return callback_id;
}

bool ConfigManager::unregisterChangeCallback(int callback_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = change_callbacks_.find(callback_id);
    if (it != change_callbacks_.end()) {
        change_callbacks_.erase(it);
        return true;
    }
    
    return false;
}

const SystemConfig& ConfigManager::getSystemConfig() const {
    return system_config_;
}

const CsmsConfig& ConfigManager::getCsmsConfig() const {
    return csms_config_;
}

const DeviceConfigCollection& ConfigManager::getDeviceConfigs() const {
    return device_configs_;
}

std::optional<DeviceConfig> ConfigManager::getDeviceConfig(const std::string& id) const {
    return device_configs_.getDevice(id);
}

bool ConfigManager::addOrUpdateDeviceConfig(const DeviceConfig& device) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Validate device configuration
        device.validate();
        
        // Add or update device configuration
        device_configs_.addDevice(device);
        
        // Save device configuration to file
        std::string device_file = config_dir_ + "/devices/" + device.getId() + ".yaml";
        
        // Ensure devices directory exists
        std::string devices_dir = config_dir_ + "/devices";
        if (!fs::exists(devices_dir)) {
            fs::create_directories(devices_dir);
        }
        
        // Create a temporary device collection with only this device
        DeviceConfigCollection temp_collection;
        temp_collection.addDevice(device);
        
        // Save to file
        if (!temp_collection.saveToYaml(device_file)) {
            return false;
        }
        
        // Notify all callbacks about configuration changes
        notifyConfigChange();
        
        return true;
    } catch (const ConfigValidationError& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool ConfigManager::removeDeviceConfig(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if device exists
    if (!device_configs_.getDevice(id)) {
        return false;
    }
    
    // Remove device configuration
    device_configs_.removeDevice(id);
    
    // Remove device configuration file
    std::string device_file = config_dir_ + "/devices/" + id + ".yaml";
    if (fs::exists(device_file)) {
        try {
            fs::remove(device_file);
        } catch (const std::exception& e) {
            // Log error
            return false;
        }
    }
    
    // Notify all callbacks about configuration changes
    notifyConfigChange();
    
    return true;
}

bool ConfigManager::saveAllConfigs() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Save system configuration
        std::string system_config_path = config_dir_ + "/system.yaml";
        if (!system_config_.saveToYaml(system_config_path)) {
            return false;
        }
        
        // Save CSMS configuration
        std::string csms_config_path = config_dir_ + "/csms.yaml";
        if (!csms_config_.saveToYaml(csms_config_path)) {
            return false;
        }
        
        // Ensure devices directory exists
        std::string devices_dir = config_dir_ + "/devices";
        if (!fs::exists(devices_dir)) {
            fs::create_directories(devices_dir);
        }
        
        // Save device configurations
        // NOLINTNEXTLINE(cppcheck-useStlAlgorithm)
        for (const auto& device : device_configs_.getDevices()) {
            std::string device_file = devices_dir + "/" + device.getId() + ".yaml";
            
            // Create a temporary device collection with only this device
            DeviceConfigCollection temp_collection;
            temp_collection.addDevice(device);
            
            // Save to file
            if (!temp_collection.saveToYaml(device_file)) {
                return false;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool ConfigManager::validateAllConfigs() const {
    try {
        // Validate system configuration
        system_config_.validate();
        
        // Validate CSMS configuration
        csms_config_.validate();
        
        // Validate device configurations
        device_configs_.validate();
        
        return true;
    } catch (const ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw ConfigValidationError(std::string("Unexpected error during validation: ") + e.what());
    }
}

void ConfigManager::notifyConfigChange() {
    for (const auto& callback : change_callbacks_) {
        try {
            callback.second();
        } catch (const std::exception& e) {
            // Log error
        }
    }
}

void ConfigManager::startFileMonitoring() {
    if (file_monitoring_active_) {
        return;
    }
    
    file_monitoring_active_ = true;
    
    // Start file monitoring in a separate thread
    std::thread([this]() {
        std::atomic<bool>& active = file_monitoring_active_;
        
        while (active) {
            // Sleep for a short time
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            // Check for file changes
            // This is a simple polling implementation
            // In a real implementation, we would use platform-specific file system monitoring APIs
            
            // For now, just reload all configurations
            reloadAllConfigs();
        }
    }).detach();
}

void ConfigManager::stopFileMonitoring() {
    file_monitoring_active_ = false;
}

void ConfigManager::handleFileChange(const std::string& file_path) {
    // This method would be called by a file system monitoring API
    // For now, we just reload all configurations
    reloadAllConfigs();
}

} // namespace config
} // namespace ocpp_gateway