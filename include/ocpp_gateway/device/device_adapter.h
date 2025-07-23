#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <future>
#include <optional>

#include "ocpp_gateway/common/error.h"
#include "ocpp_gateway/common/logger.h"

namespace ocpp_gateway {
namespace device {

/**
 * @enum DeviceProtocol
 * @brief Enumeration of supported device protocols
 */
enum class DeviceProtocol {
    ECHONET_LITE,
    MODBUS_RTU,
    MODBUS_TCP,
    UNKNOWN
};

/**
 * @brief Convert protocol string to enum
 * @param protocol_str Protocol string
 * @return DeviceProtocol enum
 */
DeviceProtocol protocolFromString(const std::string& protocol_str);

/**
 * @brief Convert protocol enum to string
 * @param protocol DeviceProtocol enum
 * @return Protocol string
 */
std::string protocolToString(DeviceProtocol protocol);

/**
 * @struct DeviceAddress
 * @brief Base structure for device addressing
 */
struct DeviceAddress {
    virtual ~DeviceAddress() = default;
};

/**
 * @struct EchonetLiteAddress
 * @brief ECHONET Lite device address
 */
struct EchonetLiteAddress : public DeviceAddress {
    std::string ip_address;
    uint16_t port = 3610; // Default ECHONET Lite port
};

/**
 * @struct ModbusRtuAddress
 * @brief Modbus RTU device address
 */
struct ModbusRtuAddress : public DeviceAddress {
    std::string port;         // Serial port (e.g., "/dev/ttyS0")
    int baud_rate = 9600;     // Baud rate
    int data_bits = 8;        // Data bits
    int stop_bits = 1;        // Stop bits
    std::string parity = "N"; // Parity (N=none, E=even, O=odd)
    int unit_id = 1;          // Modbus unit ID
};

/**
 * @struct ModbusTcpAddress
 * @brief Modbus TCP device address
 */
struct ModbusTcpAddress : public DeviceAddress {
    std::string ip_address;
    uint16_t port = 502;      // Default Modbus TCP port
    int unit_id = 1;          // Modbus unit ID
};

/**
 * @struct DeviceInfo
 * @brief Information about a device
 */
struct DeviceInfo {
    std::string id;                   // Unique device identifier
    std::string name;                 // Human-readable name
    std::string model;                // Device model
    std::string manufacturer;         // Device manufacturer
    std::string firmware_version;     // Firmware version
    DeviceProtocol protocol;          // Communication protocol
    std::shared_ptr<DeviceAddress> address; // Device address
    std::string template_id;          // ID of the mapping template to use
    bool online = false;              // Device online status
    std::chrono::system_clock::time_point last_seen; // Last time device was seen
};

/**
 * @enum RegisterType
 * @brief Types of registers for read/write operations
 */
enum class RegisterType {
    COIL,              // Modbus coil (1-bit, read-write)
    DISCRETE_INPUT,    // Modbus discrete input (1-bit, read-only)
    INPUT_REGISTER,    // Modbus input register (16-bit, read-only)
    HOLDING_REGISTER,  // Modbus holding register (16-bit, read-write)
    EPC,               // ECHONET Lite property
    UNKNOWN
};

/**
 * @enum DataType
 * @brief Data types for register values
 */
enum class DataType {
    BOOL,
    UINT8,
    INT8,
    UINT16,
    INT16,
    UINT32,
    INT32,
    UINT64,
    INT64,
    FLOAT32,
    FLOAT64,
    STRING,
    BINARY,
    UNKNOWN
};

/**
 * @struct RegisterValue
 * @brief Value of a register with type information
 */
struct RegisterValue {
    DataType type = DataType::UNKNOWN;
    std::vector<uint8_t> data;
    
    // Convenience getters for different types
    bool getBool() const;
    uint8_t getUint8() const;
    int8_t getInt8() const;
    uint16_t getUint16() const;
    int16_t getInt16() const;
    uint32_t getUint32() const;
    int32_t getInt32() const;
    uint64_t getUint64() const;
    int64_t getInt64() const;
    float getFloat32() const;
    double getFloat64() const;
    std::string getString() const;
    
    // Convenience setters for different types
    void setBool(bool value);
    void setUint8(uint8_t value);
    void setInt8(int8_t value);
    void setUint16(uint16_t value);
    void setInt16(int16_t value);
    void setUint32(uint32_t value);
    void setInt32(int32_t value);
    void setUint64(uint64_t value);
    void setInt64(int64_t value);
    void setFloat32(float value);
    void setFloat64(double value);
    void setString(const std::string& value);
    void setBinary(const std::vector<uint8_t>& value);
};

/**
 * @struct RegisterAddress
 * @brief Address of a register
 */
struct RegisterAddress {
    RegisterType type = RegisterType::UNKNOWN;
    uint32_t address = 0;
    
    // For ECHONET Lite
    uint8_t eoj_class_group_code = 0;
    uint8_t eoj_class_code = 0;
    uint8_t eoj_instance_code = 0;
    uint8_t epc = 0;
    
    // For multi-register values
    uint16_t count = 1;
    
    // Comparison operators for std::map
    bool operator<(const RegisterAddress& other) const {
        if (type != other.type) return type < other.type;
        if (address != other.address) return address < other.address;
        if (eoj_class_group_code != other.eoj_class_group_code) return eoj_class_group_code < other.eoj_class_group_code;
        if (eoj_class_code != other.eoj_class_code) return eoj_class_code < other.eoj_class_code;
        if (eoj_instance_code != other.eoj_instance_code) return eoj_instance_code < other.eoj_instance_code;
        if (epc != other.epc) return epc < other.epc;
        return count < other.count;
    }
    
    bool operator==(const RegisterAddress& other) const {
        return type == other.type &&
               address == other.address &&
               eoj_class_group_code == other.eoj_class_group_code &&
               eoj_class_code == other.eoj_class_code &&
               eoj_instance_code == other.eoj_instance_code &&
               epc == other.epc &&
               count == other.count;
    }
};

/**
 * @struct ReadResult
 * @brief Result of a register read operation
 */
struct ReadResult {
    bool success = false;
    RegisterValue value;
    std::string error_message;
    int error_code = 0;
};

/**
 * @struct WriteResult
 * @brief Result of a register write operation
 */
struct WriteResult {
    bool success = false;
    std::string error_message;
    int error_code = 0;
};

/**
 * @brief Callback for device discovery
 */
using DeviceDiscoveryCallback = std::function<void(const DeviceInfo&)>;

/**
 * @class DeviceAdapter
 * @brief Base interface for all device adapters
 */
class DeviceAdapter {
public:
    /**
     * @brief Construct a new Device Adapter object
     */
    DeviceAdapter() = default;
    
    /**
     * @brief Destroy the Device Adapter object
     */
    virtual ~DeviceAdapter() = default;
    
    /**
     * @brief Initialize the adapter
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief Start the adapter
     * @return true if start was successful
     * @return false if start failed
     */
    virtual bool start() = 0;
    
    /**
     * @brief Stop the adapter
     */
    virtual void stop() = 0;
    
    /**
     * @brief Check if the adapter is running
     * @return true if the adapter is running
     * @return false if the adapter is stopped
     */
    virtual bool isRunning() const = 0;
    
    /**
     * @brief Get the protocol supported by this adapter
     * @return DeviceProtocol enum
     */
    virtual DeviceProtocol getProtocol() const = 0;
    
    /**
     * @brief Add a device to the adapter
     * @param device_info Device information
     * @return true if the device was added successfully
     * @return false if the device could not be added
     */
    virtual bool addDevice(const DeviceInfo& device_info) = 0;
    
    /**
     * @brief Remove a device from the adapter
     * @param device_id Device ID
     * @return true if the device was removed
     * @return false if the device was not found
     */
    virtual bool removeDevice(const std::string& device_id) = 0;
    
    /**
     * @brief Get information about a device
     * @param device_id Device ID
     * @return DeviceInfo if the device was found, std::nullopt otherwise
     */
    virtual std::optional<DeviceInfo> getDeviceInfo(const std::string& device_id) const = 0;
    
    /**
     * @brief Get all devices managed by this adapter
     * @return Vector of device information
     */
    virtual std::vector<DeviceInfo> getAllDevices() const = 0;
    
    /**
     * @brief Start device discovery
     * @param callback Callback function to call when a device is discovered
     * @param timeout_ms Discovery timeout in milliseconds (0 for no timeout)
     * @return true if discovery was started successfully
     * @return false if discovery could not be started
     */
    virtual bool startDiscovery(DeviceDiscoveryCallback callback, 
                               std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(10000)) = 0;
    
    /**
     * @brief Stop device discovery
     */
    virtual void stopDiscovery() = 0;
    
    /**
     * @brief Check if discovery is in progress
     * @return true if discovery is in progress
     * @return false if discovery is not in progress
     */
    virtual bool isDiscoveryInProgress() const = 0;
    
    /**
     * @brief Read a register from a device
     * @param device_id Device ID
     * @param address Register address
     * @return ReadResult containing the register value or error information
     */
    virtual ReadResult readRegister(const std::string& device_id, const RegisterAddress& address) = 0;
    
    /**
     * @brief Write a value to a register
     * @param device_id Device ID
     * @param address Register address
     * @param value Value to write
     * @return WriteResult containing success or error information
     */
    virtual WriteResult writeRegister(const std::string& device_id, const RegisterAddress& address, 
                                     const RegisterValue& value) = 0;
    
    /**
     * @brief Read multiple registers from a device
     * @param device_id Device ID
     * @param addresses Vector of register addresses
     * @return Map of register addresses to read results
     */
    virtual std::map<RegisterAddress, ReadResult> readMultipleRegisters(
        const std::string& device_id, const std::vector<RegisterAddress>& addresses) = 0;
    
    /**
     * @brief Write multiple registers to a device
     * @param device_id Device ID
     * @param values Map of register addresses to values
     * @return Map of register addresses to write results
     */
    virtual std::map<RegisterAddress, WriteResult> writeMultipleRegisters(
        const std::string& device_id, const std::map<RegisterAddress, RegisterValue>& values) = 0;
    
    /**
     * @brief Read a register asynchronously
     * @param device_id Device ID
     * @param address Register address
     * @return Future that will contain the read result
     */
    virtual std::future<ReadResult> readRegisterAsync(const std::string& device_id, 
                                                    const RegisterAddress& address) = 0;
    
    /**
     * @brief Write a register asynchronously
     * @param device_id Device ID
     * @param address Register address
     * @param value Value to write
     * @return Future that will contain the write result
     */
    virtual std::future<WriteResult> writeRegisterAsync(const std::string& device_id, 
                                                      const RegisterAddress& address, 
                                                      const RegisterValue& value) = 0;
    
    /**
     * @brief Check if a device is online
     * @param device_id Device ID
     * @return true if the device is online
     * @return false if the device is offline or not found
     */
    virtual bool isDeviceOnline(const std::string& device_id) const = 0;
    
    /**
     * @brief Set a callback to be called when a device goes online or offline
     * @param device_id Device ID
     * @param callback Callback function
     * @return true if the callback was set successfully
     * @return false if the device was not found
     */
    virtual bool setDeviceStatusCallback(const std::string& device_id, 
                                        std::function<void(const std::string&, bool)> callback) = 0;
};

/**
 * @class BaseDeviceAdapter
 * @brief Base implementation of DeviceAdapter with common functionality
 */
class BaseDeviceAdapter : public DeviceAdapter {
public:
    /**
     * @brief Construct a new Base Device Adapter object
     * @param protocol Protocol supported by this adapter
     */
    explicit BaseDeviceAdapter(DeviceProtocol protocol);
    
    /**
     * @brief Destroy the Base Device Adapter object
     */
    ~BaseDeviceAdapter() override;
    
    /**
     * @brief Initialize the adapter
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize() override;
    
    /**
     * @brief Start the adapter
     * @return true if start was successful
     * @return false if start failed
     */
    bool start() override;
    
    /**
     * @brief Stop the adapter
     */
    void stop() override;
    
    /**
     * @brief Check if the adapter is running
     * @return true if the adapter is running
     * @return false if the adapter is stopped
     */
    bool isRunning() const override;
    
    /**
     * @brief Get the protocol supported by this adapter
     * @return DeviceProtocol enum
     */
    DeviceProtocol getProtocol() const override;
    
    /**
     * @brief Add a device to the adapter
     * @param device_info Device information
     * @return true if the device was added successfully
     * @return false if the device could not be added
     */
    bool addDevice(const DeviceInfo& device_info) override;
    
    /**
     * @brief Remove a device from the adapter
     * @param device_id Device ID
     * @return true if the device was removed
     * @return false if the device was not found
     */
    bool removeDevice(const std::string& device_id) override;
    
    /**
     * @brief Get information about a device
     * @param device_id Device ID
     * @return DeviceInfo if the device was found, std::nullopt otherwise
     */
    std::optional<DeviceInfo> getDeviceInfo(const std::string& device_id) const override;
    
    /**
     * @brief Get all devices managed by this adapter
     * @return Vector of device information
     */
    std::vector<DeviceInfo> getAllDevices() const override;
    
    /**
     * @brief Check if a device is online
     * @param device_id Device ID
     * @return true if the device is online
     * @return false if the device is offline or not found
     */
    bool isDeviceOnline(const std::string& device_id) const override;
    
    /**
     * @brief Set a callback to be called when a device goes online or offline
     * @param device_id Device ID
     * @param callback Callback function
     * @return true if the callback was set successfully
     * @return false if the device was not found
     */
    bool setDeviceStatusCallback(const std::string& device_id, 
                                std::function<void(const std::string&, bool)> callback) override;
    
    /**
     * @brief Read multiple registers from a device
     * @param device_id Device ID
     * @param addresses Vector of register addresses
     * @return Map of register addresses to read results
     */
    std::map<RegisterAddress, ReadResult> readMultipleRegisters(
        const std::string& device_id, const std::vector<RegisterAddress>& addresses) override;
    
    /**
     * @brief Write multiple registers to a device
     * @param device_id Device ID
     * @param values Map of register addresses to values
     * @return Map of register addresses to write results
     */
    std::map<RegisterAddress, WriteResult> writeMultipleRegisters(
        const std::string& device_id, const std::map<RegisterAddress, RegisterValue>& values) override;
    
    /**
     * @brief Read a register asynchronously
     * @param device_id Device ID
     * @param address Register address
     * @return Future that will contain the read result
     */
    std::future<ReadResult> readRegisterAsync(const std::string& device_id, 
                                            const RegisterAddress& address) override;
    
    /**
     * @brief Write a register asynchronously
     * @param device_id Device ID
     * @param address Register address
     * @param value Value to write
     * @return Future that will contain the write result
     */
    std::future<WriteResult> writeRegisterAsync(const std::string& device_id, 
                                              const RegisterAddress& address, 
                                              const RegisterValue& value) override;

protected:
    /**
     * @brief Update device status
     * @param device_id Device ID
     * @param online Online status
     */
    void updateDeviceStatus(const std::string& device_id, bool online);
    
    /**
     * @brief Validate device address for the protocol
     * @param device_info Device information
     * @return true if the address is valid
     * @return false if the address is invalid
     */
    virtual bool validateDeviceAddress(const DeviceInfo& device_info) const;
    
    /**
     * @brief Validate register address for the protocol
     * @param address Register address
     * @return true if the address is valid
     * @return false if the address is invalid
     */
    virtual bool validateRegisterAddress(const RegisterAddress& address) const;
    
    /**
     * @brief Validate register value for the protocol
     * @param address Register address
     * @param value Register value
     * @return true if the value is valid
     * @return false if the value is invalid
     */
    virtual bool validateRegisterValue(const RegisterAddress& address, const RegisterValue& value) const;
    
    /**
     * @brief Create a read result with an error
     * @param error_message Error message
     * @param error_code Error code
     * @return ReadResult with error information
     */
    static ReadResult createErrorReadResult(const std::string& error_message, int error_code = 0);
    
    /**
     * @brief Create a write result with an error
     * @param error_message Error message
     * @param error_code Error code
     * @return WriteResult with error information
     */
    static WriteResult createErrorWriteResult(const std::string& error_message, int error_code = 0);
    
    /**
     * @brief Create a successful read result
     * @param value Register value
     * @return ReadResult with success information
     */
    static ReadResult createSuccessReadResult(const RegisterValue& value);
    
    /**
     * @brief Create a successful write result
     * @return WriteResult with success information
     */
    static WriteResult createSuccessWriteResult();

private:
    DeviceProtocol protocol_;
    std::atomic<bool> running_{false};
    std::atomic<bool> discovery_in_progress_{false};
    std::map<std::string, DeviceInfo> devices_;
    std::map<std::string, std::function<void(const std::string&, bool)>> status_callbacks_;
    mutable std::mutex devices_mutex_;
    mutable std::mutex callbacks_mutex_;
};

} // namespace device
} // namespace ocpp_gateway