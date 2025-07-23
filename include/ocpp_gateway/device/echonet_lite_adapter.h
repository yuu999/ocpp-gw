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

namespace ocpp_gateway {
namespace device {

/**
 * @struct EchonetLiteFrame
 * @brief Structure representing an ECHONET Lite frame
 */
struct EchonetLiteFrame {
    // ECHONET Lite header (EHD)
    uint8_t ehd1 = 0x10; // ECHONET Lite message header 1
    uint8_t ehd2 = 0x81; // ECHONET Lite message header 2 (Format 1)
    
    // Transaction ID (TID)
    uint16_t tid = 0;
    
    // ECHONET Lite service header (EDATA)
    uint8_t seoj_class_group_code = 0x05; // Management/control-related device class group (default: controller)
    uint8_t seoj_class_code = 0xFF;       // Controller class
    uint8_t seoj_instance_code = 0x01;    // Instance code
    
    uint8_t deoj_class_group_code = 0x00; // Destination object class group code
    uint8_t deoj_class_code = 0x00;       // Destination object class code
    uint8_t deoj_instance_code = 0x00;    // Destination object instance code
    
    uint8_t esv = 0x00;                   // ECHONET Lite service code
    uint8_t opc = 0x00;                   // Number of processing properties
    
    // Property data
    struct Property {
        uint8_t epc = 0x00;               // ECHONET property code
        uint8_t pdc = 0x00;               // Property data counter
        std::vector<uint8_t> edt;         // Property value data
    };
    
    std::vector<Property> properties;
    
    // Serialize frame to binary data
    std::vector<uint8_t> serialize() const;
    
    // Deserialize binary data to frame
    static std::optional<EchonetLiteFrame> deserialize(const std::vector<uint8_t>& data);
    
    // Helper methods for common operations
    static EchonetLiteFrame createGetPropertyFrame(
        uint8_t deoj_class_group_code, uint8_t deoj_class_code, uint8_t deoj_instance_code, 
        const std::vector<uint8_t>& properties);
        
    static EchonetLiteFrame createSetPropertyFrame(
        uint8_t deoj_class_group_code, uint8_t deoj_class_code, uint8_t deoj_instance_code, 
        uint8_t epc, const std::vector<uint8_t>& edt);
        
    static EchonetLiteFrame createDiscoveryFrame();
};

/**
 * @struct EchonetLiteResponse
 * @brief Structure representing a response to an ECHONET Lite request
 */
struct EchonetLiteResponse {
    uint16_t tid;
    EchonetLiteFrame frame;
    bool success;
    std::string error_message;
};

/**
 * @class EchonetLiteAdapter
 * @brief Adapter for ECHONET Lite devices
 */
class EchonetLiteAdapter : public BaseDeviceAdapter {
public:
    /**
     * @brief Construct a new ECHONET Lite Adapter object
     */
    EchonetLiteAdapter();
    
    /**
     * @brief Destroy the ECHONET Lite Adapter object
     */
    ~EchonetLiteAdapter() override;
    
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
    // Socket management
    int socket_fd_ = -1;
    int multicast_socket_fd_ = -1;
    
    // Thread for receiving responses
    std::thread receiver_thread_;
    std::atomic<bool> receiver_running_{false};
    
    // Thread for discovery
    std::thread discovery_thread_;
    std::atomic<bool> discovery_running_{false};
    DeviceDiscoveryCallback discovery_callback_;
    std::chrono::milliseconds discovery_timeout_{10000};
    std::chrono::time_point<std::chrono::steady_clock> discovery_start_time_;
    
    // Pending requests
    struct PendingRequest {
        uint16_t tid;
        std::chrono::time_point<std::chrono::steady_clock> timestamp;
        std::chrono::milliseconds timeout;
        std::function<void(const EchonetLiteResponse&)> callback;
    };
    
    std::mutex pending_requests_mutex_;
    std::map<uint16_t, PendingRequest> pending_requests_;
    
    // Discovered devices during discovery
    std::mutex discovered_devices_mutex_;
    std::set<std::string> discovered_devices_;
    
    // Next transaction ID
    std::atomic<uint16_t> next_tid_{1};
    
    // Helper methods
    bool initializeSocket();
    bool initializeMulticastSocket();
    void closeSocket();
    void closeMulticastSocket();
    
    void receiverThreadFunc();
    void discoveryThreadFunc();
    
    bool sendFrame(const std::string& device_id, const EchonetLiteFrame& frame);
    bool sendMulticastFrame(const EchonetLiteFrame& frame);
    
    std::optional<EchonetLiteResponse> sendRequestWithResponse(
        const std::string& device_id, 
        const EchonetLiteFrame& frame, 
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    
    void handleReceivedFrame(const std::string& source_ip, const EchonetLiteFrame& frame);
    void handleDiscoveryResponse(const std::string& source_ip, const EchonetLiteFrame& frame);
    
    uint16_t getNextTid();
    
    // Device status monitoring
    void startDeviceStatusMonitoring();
    void stopDeviceStatusMonitoring();
    std::thread status_monitor_thread_;
    std::atomic<bool> status_monitor_running_{false};
    void statusMonitorThreadFunc();
    
    // Retransmission management
    bool retransmitRequest(const std::string& device_id, const EchonetLiteFrame& frame, 
                          uint16_t tid, int max_retries = 3);
};

// ECHONET Lite EV charger classes (corrected based on ECHONET specification)
constexpr uint8_t EOJ_EV_CHARGER_CLASS_GROUP = 0x02;  // 住宅・設備関連機器クラスグループ
constexpr uint8_t EOJ_EV_CHARGER_CLASS = 0xA1;        // 電気自動車充電器クラス
constexpr uint8_t EOJ_EV_DISCHARGER_CLASS = 0xA1;     // 電気自動車充電器クラス（充放電対応）

} // namespace device
} // namespace ocpp_gateway