#include "ocpp_gateway/device/device_adapter.h"
#include <algorithm>
#include <cctype>

namespace ocpp_gateway {
namespace device {

DeviceProtocol protocolFromString(const std::string& protocol_str) {
    std::string lower_str = protocol_str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    
    if (lower_str == "echonet_lite" || lower_str == "echonetlite" || lower_str == "el") {
        return DeviceProtocol::ECHONET_LITE;
    } else if (lower_str == "modbus_rtu" || lower_str == "modbusrtu") {
        return DeviceProtocol::MODBUS_RTU;
    } else if (lower_str == "modbus_tcp" || lower_str == "modbustcp") {
        return DeviceProtocol::MODBUS_TCP;
    }
    
    return DeviceProtocol::UNKNOWN;
}

std::string protocolToString(DeviceProtocol protocol) {
    switch (protocol) {
        case DeviceProtocol::ECHONET_LITE:
            return "ECHONET_LITE";
        case DeviceProtocol::MODBUS_RTU:
            return "MODBUS_RTU";
        case DeviceProtocol::MODBUS_TCP:
            return "MODBUS_TCP";
        default:
            return "UNKNOWN";
    }
}

// RegisterValue implementation

bool RegisterValue::getBool() const {
    if (data.empty()) {
        return false;
    }
    return data[0] != 0;
}

uint8_t RegisterValue::getUint8() const {
    if (data.empty()) {
        return 0;
    }
    return data[0];
}

int8_t RegisterValue::getInt8() const {
    if (data.empty()) {
        return 0;
    }
    return static_cast<int8_t>(data[0]);
}

uint16_t RegisterValue::getUint16() const {
    if (data.size() < 2) {
        return 0;
    }
    return static_cast<uint16_t>((data[0] << 8) | data[1]);
}

int16_t RegisterValue::getInt16() const {
    return static_cast<int16_t>(getUint16());
}

uint32_t RegisterValue::getUint32() const {
    if (data.size() < 4) {
        return 0;
    }
    return static_cast<uint32_t>((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
}

int32_t RegisterValue::getInt32() const {
    return static_cast<int32_t>(getUint32());
}

uint64_t RegisterValue::getUint64() const {
    if (data.size() < 8) {
        return 0;
    }
    uint64_t result = 0;
    for (int i = 0; i < 8; ++i) {
        result = (result << 8) | data[i];
    }
    return result;
}

int64_t RegisterValue::getInt64() const {
    return static_cast<int64_t>(getUint64());
}

float RegisterValue::getFloat32() const {
    if (data.size() < 4) {
        return 0.0f;
    }
    
    // IEEE 754 float conversion
    uint32_t bits = getUint32();
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

double RegisterValue::getFloat64() const {
    if (data.size() < 8) {
        return 0.0;
    }
    
    // IEEE 754 double conversion
    uint64_t bits = getUint64();
    double result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

std::string RegisterValue::getString() const {
    return std::string(data.begin(), data.end());
}

void RegisterValue::setBool(bool value) {
    type = DataType::BOOL;
    data.resize(1);
    data[0] = value ? 1 : 0;
}

void RegisterValue::setUint8(uint8_t value) {
    type = DataType::UINT8;
    data.resize(1);
    data[0] = value;
}

void RegisterValue::setInt8(int8_t value) {
    type = DataType::INT8;
    data.resize(1);
    data[0] = static_cast<uint8_t>(value);
}

void RegisterValue::setUint16(uint16_t value) {
    type = DataType::UINT16;
    data.resize(2);
    data[0] = static_cast<uint8_t>(value >> 8);
    data[1] = static_cast<uint8_t>(value);
}

void RegisterValue::setInt16(int16_t value) {
    type = DataType::INT16;
    setUint16(static_cast<uint16_t>(value));
}

void RegisterValue::setUint32(uint32_t value) {
    type = DataType::UINT32;
    data.resize(4);
    data[0] = static_cast<uint8_t>(value >> 24);
    data[1] = static_cast<uint8_t>(value >> 16);
    data[2] = static_cast<uint8_t>(value >> 8);
    data[3] = static_cast<uint8_t>(value);
}

void RegisterValue::setInt32(int32_t value) {
    type = DataType::INT32;
    setUint32(static_cast<uint32_t>(value));
}

void RegisterValue::setUint64(uint64_t value) {
    type = DataType::UINT64;
    data.resize(8);
    for (int i = 0; i < 8; ++i) {
        data[i] = static_cast<uint8_t>(value >> ((7 - i) * 8));
    }
}

void RegisterValue::setInt64(int64_t value) {
    type = DataType::INT64;
    setUint64(static_cast<uint64_t>(value));
}

void RegisterValue::setFloat32(float value) {
    type = DataType::FLOAT32;
    data.resize(4);
    
    // IEEE 754 float conversion
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    
    data[0] = static_cast<uint8_t>(bits >> 24);
    data[1] = static_cast<uint8_t>(bits >> 16);
    data[2] = static_cast<uint8_t>(bits >> 8);
    data[3] = static_cast<uint8_t>(bits);
}

void RegisterValue::setFloat64(double value) {
    type = DataType::FLOAT64;
    data.resize(8);
    
    // IEEE 754 double conversion
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    
    for (int i = 0; i < 8; ++i) {
        data[i] = static_cast<uint8_t>(bits >> ((7 - i) * 8));
    }
}

void RegisterValue::setString(const std::string& value) {
    type = DataType::STRING;
    data.assign(value.begin(), value.end());
}

void RegisterValue::setBinary(const std::vector<uint8_t>& value) {
    type = DataType::BINARY;
    data = value;
}

// BaseDeviceAdapter implementation

BaseDeviceAdapter::BaseDeviceAdapter(DeviceProtocol protocol)
    : protocol_(protocol) {
}

BaseDeviceAdapter::~BaseDeviceAdapter() {
    stop();
}

bool BaseDeviceAdapter::initialize() {
    LOG_INFO("Initializing {} device adapter", protocolToString(protocol_));
    return true;
}

bool BaseDeviceAdapter::start() {
    if (running_) {
        LOG_WARN("{} device adapter already running", protocolToString(protocol_));
        return true;
    }
    
    LOG_INFO("Starting {} device adapter", protocolToString(protocol_));
    running_ = true;
    return true;
}

void BaseDeviceAdapter::stop() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Stopping {} device adapter", protocolToString(protocol_));
    
    // Stop discovery if in progress
    stopDiscovery();
    
    // Mark all devices as offline
    std::lock_guard<std::mutex> lock(devices_mutex_);
    for (auto& device_pair : devices_) {
        if (device_pair.second.online) {
            device_pair.second.online = false;
            updateDeviceStatus(device_pair.first, false);
        }
    }
    
    running_ = false;
}

bool BaseDeviceAdapter::isRunning() const {
    return running_;
}

DeviceProtocol BaseDeviceAdapter::getProtocol() const {
    return protocol_;
}

bool BaseDeviceAdapter::addDevice(const DeviceInfo& device_info) {
    if (!validateDeviceAddress(device_info)) {
        LOG_ERROR("Invalid device address for device {}", device_info.id);
        return false;
    }
    
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    // Check if device already exists
    if (devices_.find(device_info.id) != devices_.end()) {
        LOG_WARN("Device {} already exists", device_info.id);
        return false;
    }
    
    // Add device
    devices_[device_info.id] = device_info;
    LOG_INFO("Added device {} to {} adapter", device_info.id, protocolToString(protocol_));
    
    return true;
}

bool BaseDeviceAdapter::removeDevice(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = devices_.find(device_id);
    if (it == devices_.end()) {
        LOG_WARN("Device {} not found", device_id);
        return false;
    }
    
    // Remove device status callback
    {
        std::lock_guard<std::mutex> callbacks_lock(callbacks_mutex_);
        status_callbacks_.erase(device_id);
    }
    
    // Remove device
    devices_.erase(it);
    LOG_INFO("Removed device {} from {} adapter", device_id, protocolToString(protocol_));
    
    return true;
}

std::optional<DeviceInfo> BaseDeviceAdapter::getDeviceInfo(const std::string& device_id) const {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = devices_.find(device_id);
    if (it == devices_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

std::vector<DeviceInfo> BaseDeviceAdapter::getAllDevices() const {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    std::vector<DeviceInfo> result;
    result.reserve(devices_.size());
    
    for (const auto& device_pair : devices_) {
        result.push_back(device_pair.second);
    }
    
    return result;
}

bool BaseDeviceAdapter::isDeviceOnline(const std::string& device_id) const {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = devices_.find(device_id);
    if (it == devices_.end()) {
        return false;
    }
    
    return it->second.online;
}

bool BaseDeviceAdapter::setDeviceStatusCallback(const std::string& device_id, 
                                              std::function<void(const std::string&, bool)> callback) {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    if (devices_.find(device_id) == devices_.end()) {
        LOG_WARN("Device {} not found", device_id);
        return false;
    }
    
    std::lock_guard<std::mutex> callbacks_lock(callbacks_mutex_);
    status_callbacks_[device_id] = callback;
    
    return true;
}

std::map<RegisterAddress, ReadResult> BaseDeviceAdapter::readMultipleRegisters(
    const std::string& device_id, const std::vector<RegisterAddress>& addresses) {
    
    std::map<RegisterAddress, ReadResult> results;
    
    // Default implementation reads registers one by one
    for (const auto& address : addresses) {
        results[address] = readRegister(device_id, address);
    }
    
    return results;
}

std::map<RegisterAddress, WriteResult> BaseDeviceAdapter::writeMultipleRegisters(
    const std::string& device_id, const std::map<RegisterAddress, RegisterValue>& values) {
    
    std::map<RegisterAddress, WriteResult> results;
    
    // Default implementation writes registers one by one
    for (const auto& value_pair : values) {
        results[value_pair.first] = writeRegister(device_id, value_pair.first, value_pair.second);
    }
    
    return results;
}

std::future<ReadResult> BaseDeviceAdapter::readRegisterAsync(const std::string& device_id, 
                                                          const RegisterAddress& address) {
    // Default implementation runs synchronously in a new thread
    return std::async(std::launch::async, [this, device_id, address]() {
        return this->readRegister(device_id, address);
    });
}

std::future<WriteResult> BaseDeviceAdapter::writeRegisterAsync(const std::string& device_id, 
                                                            const RegisterAddress& address, 
                                                            const RegisterValue& value) {
    // Default implementation runs synchronously in a new thread
    return std::async(std::launch::async, [this, device_id, address, value]() {
        return this->writeRegister(device_id, address, value);
    });
}

void BaseDeviceAdapter::updateDeviceStatus(const std::string& device_id, bool online) {
    bool status_changed = false;
    
    {
        std::lock_guard<std::mutex> lock(devices_mutex_);
        
        auto it = devices_.find(device_id);
        if (it == devices_.end()) {
            LOG_WARN("Device {} not found", device_id);
            return;
        }
        
        if (it->second.online != online) {
            it->second.online = online;
            if (online) {
                it->second.last_seen = std::chrono::system_clock::now();
            }
            status_changed = true;
        }
    }
    
    if (status_changed) {
        LOG_INFO("Device {} is now {}", device_id, online ? "online" : "offline");
        
        // Call status callback if registered
        std::lock_guard<std::mutex> callbacks_lock(callbacks_mutex_);
        auto it = status_callbacks_.find(device_id);
        if (it != status_callbacks_.end() && it->second) {
            try {
                it->second(device_id, online);
            } catch (const std::exception& e) {
                LOG_ERROR("Exception in device status callback: {}", e.what());
            }
        }
    }
}

bool BaseDeviceAdapter::validateDeviceAddress(const DeviceInfo& device_info) const {
    // Base implementation just checks if address is not null
    return device_info.address != nullptr;
}

bool BaseDeviceAdapter::validateRegisterAddress(const RegisterAddress& address) const {
    // Base implementation just checks if register type is valid for the protocol
    switch (protocol_) {
        case DeviceProtocol::ECHONET_LITE:
            return address.type == RegisterType::EPC;
        
        case DeviceProtocol::MODBUS_RTU:
        case DeviceProtocol::MODBUS_TCP:
            return address.type == RegisterType::COIL ||
                   address.type == RegisterType::DISCRETE_INPUT ||
                   address.type == RegisterType::INPUT_REGISTER ||
                   address.type == RegisterType::HOLDING_REGISTER;
        
        default:
            return false;
    }
}

bool BaseDeviceAdapter::validateRegisterValue(const RegisterAddress& address, const RegisterValue& value) const {
    // Base implementation just checks if value is not empty
    return !value.data.empty();
}

ReadResult BaseDeviceAdapter::createErrorReadResult(const std::string& error_message, int error_code) {
    ReadResult result;
    result.success = false;
    result.error_message = error_message;
    result.error_code = error_code;
    return result;
}

WriteResult BaseDeviceAdapter::createErrorWriteResult(const std::string& error_message, int error_code) {
    WriteResult result;
    result.success = false;
    result.error_message = error_message;
    result.error_code = error_code;
    return result;
}

ReadResult BaseDeviceAdapter::createSuccessReadResult(const RegisterValue& value) {
    ReadResult result;
    result.success = true;
    result.value = value;
    return result;
}

WriteResult BaseDeviceAdapter::createSuccessWriteResult() {
    WriteResult result;
    result.success = true;
    return result;
}

} // namespace device
} // namespace ocpp_gateway