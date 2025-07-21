#pragma once

#include <string>
#include <memory>
#include <map>
#include <vector>
#include "ocpp_gateway/common/error.h"
#include "ocpp_gateway/device/device_adapter.h"

namespace ocpp_gateway {
namespace mapping {

/**
 * @brief Mapping Engine for protocol translation between OCPP and device protocols
 * 
 * This class manages the translation between OCPP variables and device-specific
 * registers (Modbus) or EPCs (ECHONET Lite). It loads mapping configurations
 * from external files and provides runtime translation capabilities.
 */
class MappingEngine {
public:
    /**
     * @brief Default constructor
     */
    MappingEngine();

    /**
     * @brief Destructor
     */
    ~MappingEngine() = default;

    /**
     * @brief Initialize the mapping engine with configuration
     * 
     * @param config_path Path to mapping configuration file
     * @return bool True if initialization was successful
     */
    bool initialize(const std::string& config_path);

    /**
     * @brief Add a device adapter for protocol translation
     * 
     * @param device_id Unique device identifier
     * @param adapter Device adapter instance
     * @return bool True if adapter was added successfully
     */
    bool addDeviceAdapter(const std::string& device_id, 
                         std::shared_ptr<device::DeviceAdapter> adapter);

    /**
     * @brief Remove a device adapter
     * 
     * @param device_id Device identifier
     * @return bool True if adapter was removed successfully
     */
    bool removeDeviceAdapter(const std::string& device_id);

    /**
     * @brief Start the mapping engine
     * 
     * @return bool True if started successfully
     */
    bool start();

    /**
     * @brief Stop the mapping engine
     */
    void stop();

    /**
     * @brief Check if the mapping engine is running
     * 
     * @return bool True if running
     */
    bool isRunning() const;

private:
    bool running_;
    std::string config_path_;
    std::map<std::string, std::shared_ptr<device::DeviceAdapter>> device_adapters_;
};

} // namespace mapping
} // namespace ocpp_gateway 