#include "ocpp_gateway/mapping/mapping_engine.h"
#include "ocpp_gateway/common/logger.h"

namespace ocpp_gateway {
namespace mapping {

MappingEngine::MappingEngine() : running_(false) {
}

bool MappingEngine::initialize(const std::string& config_path) {
    config_path_ = config_path;
    LOG_INFO("Mapping engine initialized with config: {}", config_path);
    return true;
}

bool MappingEngine::addDeviceAdapter(const std::string& device_id, 
                                    std::shared_ptr<device::DeviceAdapter> adapter) {
    if (!adapter) {
        LOG_ERROR("Cannot add null device adapter for device: {}", device_id);
        return false;
    }
    
    device_adapters_[device_id] = adapter;
    LOG_INFO("Added device adapter for device: {}", device_id);
    return true;
}

bool MappingEngine::removeDeviceAdapter(const std::string& device_id) {
    auto it = device_adapters_.find(device_id);
    if (it == device_adapters_.end()) {
        LOG_WARN("Device adapter not found for device: {}", device_id);
        return false;
    }
    
    device_adapters_.erase(it);
    LOG_INFO("Removed device adapter for device: {}", device_id);
    return true;
}

bool MappingEngine::start() {
    if (running_) {
        LOG_WARN("Mapping engine is already running");
        return true;
    }
    
    running_ = true;
    LOG_INFO("Mapping engine started");
    return true;
}

void MappingEngine::stop() {
    if (!running_) {
        LOG_WARN("Mapping engine is not running");
        return;
    }
    
    running_ = false;
    LOG_INFO("Mapping engine stopped");
}

bool MappingEngine::isRunning() const {
    return running_;
}

} // namespace mapping
} // namespace ocpp_gateway 