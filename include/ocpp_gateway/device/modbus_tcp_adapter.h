#pragma once

#include "ocpp_gateway/device/device_adapter.h"
#include "ocpp_gateway/common/logger.h"
#include <atomic>
#include <thread>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <set>
#include <future>

// Forward declaration for libmodbus context
typedef struct _modbus modbus_t;

namespace ocpp_gateway {
namespace device {

/**
 * @class ModbusTcpAdapter
 * @brief Adapter for Modbus TCP devices
 */
class ModbusTcpAdapter : public BaseDeviceAdapter {
public:
    /**
     * @brief Construct a new Modbus TCP Adapter object
     */
    ModbusTcpAdapter();
    
    /**
     * @brief Destroy the Modbus TCP Adapter object
     */
    ~ModbusTcpAdapter() override;
    
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
     * @brief Start device discovery
     * @param callback Callback function to call when a device is discovered
     * @param timeout_ms Discovery timeout in milliseconds (0 for no timeout)
     * @return true if discovery was started successfully
     * @return false if discovery could not be started
     */
    bool startDiscovery(DeviceDiscoveryCallback callback, 
                       std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(10000)) override;
    
    /**
     * @brief Stop device discovery
     */
    void stopDiscovery() override;
    
    /**
     * @brief Check if discovery is in progress
     * @return true if discovery is in progress
     * @return false if discovery is not in progress
     */
    bool isDiscoveryInProgress() const override;
    
    /**
     * @brief Read a register from a device
     * @param device_id Device ID
     * @param address Register address
     * @return ReadResult containing the register value or error information
     */
    ReadResult readRegister(const std::string& device_id, const RegisterAddress& address) override;
    
    /**
     * @brief Write a value to a register
     * @param device_id Device ID
     * @param address Register address
     * @param value Value to write
     * @return WriteResult containing success or error information
     */
    WriteResult writeRegister(const std::string& device_id, const RegisterAddress& address, 
                             const RegisterValue& value) override;
    
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

protected:
    /**
     * @brief Validate device address for the protocol
     * @param device_info Device information
     * @return true if the address is valid
     * @return false if the address is invalid
     */
    bool validateDeviceAddress(const DeviceInfo& device_info) const override;
    
    /**
     * @brief Validate register address for the protocol
     * @param address Register address
     * @return true if the address is valid
     * @return false if the address is invalid
     */
    bool validateRegisterAddress(const RegisterAddress& address) const override;
    
    /**
     * @brief Validate register value for the protocol
     * @param address Register address
     * @param value Register value
     * @return true if the value is valid
     * @return false if the value is invalid
     */
    bool validateRegisterValue(const RegisterAddress& address, const RegisterValue& value) const override;

private:
    // Structure to hold Modbus connection information
    struct ModbusConnection {
        std::string ip_address;
        uint16_t port;
        modbus_t* ctx;
        std::mutex mutex;
        std::chrono::time_point<std::chrono::steady_clock> last_used;
        std::set<int> unit_ids; // Unit IDs using this connection
        bool connected;
        std::chrono::time_point<std::chrono::steady_clock> last_keepalive;
    };
    
    // Connection pool
    struct ConnectionPool {
        std::map<std::string, std::shared_ptr<ModbusConnection>> connections; // Key: "ip:port"
        std::mutex mutex;
        size_t max_connections = 10; // Maximum number of connections in the pool
        std::chrono::seconds connection_timeout{300}; // Connection timeout (5 minutes)
        std::chrono::seconds keepalive_interval{60}; // Keep-alive interval (1 minute)
    };
    
    ConnectionPool connection_pool_;
    
    // Thread for polling devices
    std::thread polling_thread_;
    std::atomic<bool> polling_running_{false};
    
    // Thread for discovery
    std::thread discovery_thread_;
    std::atomic<bool> discovery_running_{false};
    DeviceDiscoveryCallback discovery_callback_;
    std::chrono::milliseconds discovery_timeout_{10000};
    std::chrono::time_point<std::chrono::steady_clock> discovery_start_time_;
    
    // Thread for connection management (keep-alive, cleanup)
    std::thread connection_manager_thread_;
    std::atomic<bool> connection_manager_running_{false};
    
    // Polling configuration
    struct PollingConfig {
        std::chrono::milliseconds interval{5000}; // Default polling interval: 5 seconds
        std::vector<RegisterAddress> addresses;   // Addresses to poll
    };
    
    std::map<std::string, PollingConfig> polling_configs_;
    std::mutex polling_configs_mutex_;
    
    // Helper methods
    std::shared_ptr<ModbusConnection> getConnection(const std::string& device_id);
    std::shared_ptr<ModbusConnection> createConnection(const ModbusTcpAddress& address);
    void closeConnection(std::shared_ptr<ModbusConnection> connection);
    std::string getConnectionKey(const std::string& ip_address, uint16_t port);
    
    void pollingThreadFunc();
    void discoveryThreadFunc();
    void connectionManagerThreadFunc();
    
    ReadResult readCoil(std::shared_ptr<ModbusConnection> connection, int unit_id, const RegisterAddress& address);
    ReadResult readDiscreteInput(std::shared_ptr<ModbusConnection> connection, int unit_id, const RegisterAddress& address);
    ReadResult readInputRegister(std::shared_ptr<ModbusConnection> connection, int unit_id, const RegisterAddress& address);
    ReadResult readHoldingRegister(std::shared_ptr<ModbusConnection> connection, int unit_id, const RegisterAddress& address);
    
    WriteResult writeCoil(std::shared_ptr<ModbusConnection> connection, int unit_id, const RegisterAddress& address, const RegisterValue& value);
    WriteResult writeHoldingRegister(std::shared_ptr<ModbusConnection> connection, int unit_id, const RegisterAddress& address, const RegisterValue& value);
    
    // Convert between Modbus register values and RegisterValue
    RegisterValue convertToRegisterValue(uint16_t* data, size_t count, DataType type);
    std::vector<uint16_t> convertFromRegisterValue(const RegisterValue& value, size_t& count);
    
    // Group registers by type and address for batch operations
    struct RegisterGroup {
        RegisterType type;
        uint32_t start_address;
        uint16_t count;
        std::vector<RegisterAddress> addresses;
    };
    
    std::vector<RegisterGroup> groupRegisters(const std::vector<RegisterAddress>& addresses);
    
    // Error handling
    ReadResult handleModbusError(const std::string& operation, int error_code);
    WriteResult handleModbusError(const std::string& operation, int error_code);
    
    // Device status monitoring
    void startDeviceStatusMonitoring();
    void stopDeviceStatusMonitoring();
    std::thread status_monitor_thread_;
    std::atomic<bool> status_monitor_running_{false};
    void statusMonitorThreadFunc();
    
    // Keep-alive mechanism
    bool sendKeepAlive(std::shared_ptr<ModbusConnection> connection);
    
    // Retry configuration
    int max_retries_ = 3;
    std::chrono::milliseconds retry_delay_{100};
    
    // Asynchronous operation support
    std::mutex async_mutex_;
    std::condition_variable async_cv_;
    std::queue<std::function<void()>> async_tasks_;
    std::thread async_thread_;
    std::atomic<bool> async_running_{false};
    void asyncThreadFunc();
    void queueAsyncTask(std::function<void()> task);
};

} // namespace device
} // namespace ocpp_gateway