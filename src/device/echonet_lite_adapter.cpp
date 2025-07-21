#include "ocpp_gateway/device/echonet_lite_adapter.h"
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <random>

namespace ocpp_gateway {
namespace device {

// ECHONET Lite ESV (ECHONET Lite Service) codes
constexpr uint8_t ESV_GET_REQUEST = 0x62;              // Property value read request
constexpr uint8_t ESV_GET_RESPONSE = 0x72;             // Property value read response
constexpr uint8_t ESV_SET_REQUEST = 0x61;              // Property value write request
constexpr uint8_t ESV_SET_RESPONSE = 0x71;             // Property value write response
constexpr uint8_t ESV_INF_REQUEST = 0x63;              // Property value notification request
constexpr uint8_t ESV_INF = 0x73;                      // Property value notification
constexpr uint8_t ESV_GET_SNA = 0x52;                  // Property value read request not possible response
constexpr uint8_t ESV_SET_SNA = 0x51;                  // Property value write request not possible response
constexpr uint8_t ESV_SETI_SNA = 0x55;                 // Property value write & read request not possible response

// ECHONET Lite multicast address and port
constexpr const char* ECHONET_MULTICAST_ADDR = "224.0.23.0";
constexpr uint16_t ECHONET_PORT = 3610;

// ECHONET Lite node profile object
constexpr uint8_t EOJ_NODE_PROFILE_CLASS_GROUP = 0x0E;
constexpr uint8_t EOJ_NODE_PROFILE_CLASS = 0xF0;
constexpr uint8_t EOJ_NODE_PROFILE_INSTANCE = 0x01;

// ECHONET Lite node profile properties
constexpr uint8_t EPC_INSTANCE_LIST_NOTIFICATION = 0xD5;
constexpr uint8_t EPC_SELF_NODE_INSTANCE_LIST_S = 0xD6;

// ECHONET Lite EV charger classes (corrected based on ECHONET specification)
constexpr uint8_t EOJ_EV_CHARGER_CLASS_GROUP = 0x02;  // 住宅・設備関連機器クラスグループ
constexpr uint8_t EOJ_EV_CHARGER_CLASS = 0xA1;        // 電気自動車充電器クラス
constexpr uint8_t EOJ_EV_DISCHARGER_CLASS = 0xA1;     // 電気自動車充電器クラス（充放電対応）
constexpr uint8_t EOJ_STORAGE_BATTERY_CLASS = 0x87;   // 蓄電池クラス
constexpr uint8_t EOJ_PV_POWER_GENERATION_CLASS = 0x88; // 太陽光発電クラス

// ECHONET Lite common properties
constexpr uint8_t EPC_OPERATION_STATUS = 0x80;
constexpr uint8_t EPC_INSTALLATION_LOCATION = 0x81;
constexpr uint8_t EPC_STANDARD_VERSION = 0x82;
constexpr uint8_t EPC_FAULT_STATUS = 0x88;
constexpr uint8_t EPC_MANUFACTURER_CODE = 0x8A;

// ECHONET Lite EV charger specific properties
constexpr uint8_t EPC_OPERATION_MODE_SETTING = 0xDA;  // 運転モード設定

// Socket timeout in milliseconds
constexpr int SOCKET_TIMEOUT_MS = 1000;

// Maximum UDP packet size
constexpr size_t MAX_UDP_PACKET_SIZE = 1500;

// Status monitoring interval in milliseconds
constexpr int STATUS_MONITOR_INTERVAL_MS = 30000;

// Helper function to convert bytes to hex string for logging
std::string bytesToHexString(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : bytes) {
        ss << std::setw(2) << static_cast<int>(byte) << " ";
    }
    return ss.str();
}

// EchonetLiteFrame implementation

std::vector<uint8_t> EchonetLiteFrame::serialize() const {
    std::vector<uint8_t> data;
    
    // EHD (ECHONET Lite message header)
    data.push_back(ehd1);
    data.push_back(ehd2);
    
    // TID (Transaction ID)
    data.push_back(static_cast<uint8_t>(tid >> 8));
    data.push_back(static_cast<uint8_t>(tid & 0xFF));
    
    // SEOJ (Source ECHONET Lite object)
    data.push_back(seoj_class_group_code);
    data.push_back(seoj_class_code);
    data.push_back(seoj_instance_code);
    
    // DEOJ (Destination ECHONET Lite object)
    data.push_back(deoj_class_group_code);
    data.push_back(deoj_class_code);
    data.push_back(deoj_instance_code);
    
    // ESV (ECHONET Lite service)
    data.push_back(esv);
    
    // OPC (Number of processing properties)
    data.push_back(static_cast<uint8_t>(properties.size()));
    
    // Property data
    for (const auto& prop : properties) {
        data.push_back(prop.epc);
        data.push_back(prop.pdc);
        data.insert(data.end(), prop.edt.begin(), prop.edt.end());
    }
    
    return data;
}

std::optional<EchonetLiteFrame> EchonetLiteFrame::deserialize(const std::vector<uint8_t>& data) {
    // Check minimum frame size (EHD1 + EHD2 + TID + SEOJ + DEOJ + ESV + OPC)
    if (data.size() < 12) {
        return std::nullopt;
    }
    
    EchonetLiteFrame frame;
    size_t pos = 0;
    
    // Parse header
    frame.ehd1 = data[pos++];
    frame.ehd2 = data[pos++];
    
    // Check if this is a valid ECHONET Lite frame
    if (frame.ehd1 != 0x10 || (frame.ehd2 != 0x81 && frame.ehd2 != 0x82)) {
        return std::nullopt;
    }
    
    // Parse TID
    frame.tid = (static_cast<uint16_t>(data[pos]) << 8) | data[pos + 1];
    pos += 2;
    
    // Parse SEOJ
    frame.seoj_class_group_code = data[pos++];
    frame.seoj_class_code = data[pos++];
    frame.seoj_instance_code = data[pos++];
    
    // Parse DEOJ
    frame.deoj_class_group_code = data[pos++];
    frame.deoj_class_code = data[pos++];
    frame.deoj_instance_code = data[pos++];
    
    // Parse ESV
    frame.esv = data[pos++];
    
    // Parse OPC
    frame.opc = data[pos++];
    
    // Parse properties
    for (uint8_t i = 0; i < frame.opc; ++i) {
        // Check if we have enough data for EPC and PDC
        if (pos + 2 > data.size()) {
            return std::nullopt;
        }
        
        EchonetLiteFrame::Property prop;
        prop.epc = data[pos++];
        prop.pdc = data[pos++];
        
        // Check if we have enough data for EDT
        if (pos + prop.pdc > data.size()) {
            return std::nullopt;
        }
        
        // Copy EDT
        prop.edt.assign(data.begin() + pos, data.begin() + pos + prop.pdc);
        pos += prop.pdc;
        
        frame.properties.push_back(prop);
    }
    
    return frame;
}

EchonetLiteFrame EchonetLiteFrame::createGetPropertyFrame(
    uint8_t deoj_class_group_code, uint8_t deoj_class_code, uint8_t deoj_instance_code, 
    const std::vector<uint8_t>& properties) {
    
    EchonetLiteFrame frame;
    frame.deoj_class_group_code = deoj_class_group_code;
    frame.deoj_class_code = deoj_class_code;
    frame.deoj_instance_code = deoj_instance_code;
    frame.esv = ESV_GET_REQUEST;
    
    for (const auto& epc : properties) {
        EchonetLiteFrame::Property prop;
        prop.epc = epc;
        prop.pdc = 0; // No data for GET request
        frame.properties.push_back(prop);
    }
    
    return frame;
}

EchonetLiteFrame EchonetLiteFrame::createSetPropertyFrame(
    uint8_t deoj_class_group_code, uint8_t deoj_class_code, uint8_t deoj_instance_code, 
    uint8_t epc, const std::vector<uint8_t>& edt) {
    
    EchonetLiteFrame frame;
    frame.deoj_class_group_code = deoj_class_group_code;
    frame.deoj_class_code = deoj_class_code;
    frame.deoj_instance_code = deoj_instance_code;
    frame.esv = ESV_SET_REQUEST;
    
    EchonetLiteFrame::Property prop;
    prop.epc = epc;
    prop.pdc = static_cast<uint8_t>(edt.size());
    prop.edt = edt;
    frame.properties.push_back(prop);
    
    return frame;
}

EchonetLiteFrame EchonetLiteFrame::createDiscoveryFrame() {
    // Create a frame to get the instance list
    std::vector<uint8_t> properties = {EPC_SELF_NODE_INSTANCE_LIST_S};
    return createGetPropertyFrame(EOJ_NODE_PROFILE_CLASS_GROUP, EOJ_NODE_PROFILE_CLASS, 
                                 EOJ_NODE_PROFILE_INSTANCE, properties);
}

// EchonetLiteAdapter implementation

EchonetLiteAdapter::EchonetLiteAdapter()
    : BaseDeviceAdapter(DeviceProtocol::ECHONET_LITE) {
}

EchonetLiteAdapter::~EchonetLiteAdapter() {
    stop();
}

bool EchonetLiteAdapter::initialize() {
    if (!BaseDeviceAdapter::initialize()) {
        return false;
    }
    
    LOG_INFO("Initializing ECHONET Lite adapter");
    
    if (!initializeSocket()) {
        LOG_ERROR("Failed to initialize ECHONET Lite socket");
        return false;
    }
    
    if (!initializeMulticastSocket()) {
        LOG_ERROR("Failed to initialize ECHONET Lite multicast socket");
        closeSocket();
        return false;
    }
    
    return true;
}

bool EchonetLiteAdapter::start() {
    if (!BaseDeviceAdapter::start()) {
        return false;
    }
    
    LOG_INFO("Starting ECHONET Lite adapter");
    
    // Start receiver thread
    receiver_running_ = true;
    receiver_thread_ = std::thread(&EchonetLiteAdapter::receiverThreadFunc, this);
    
    // Start device status monitoring
    startDeviceStatusMonitoring();
    
    return true;
}

void EchonetLiteAdapter::stop() {
    if (!isRunning()) {
        return;
    }
    
    LOG_INFO("Stopping ECHONET Lite adapter");
    
    // Stop discovery if in progress
    stopDiscovery();
    
    // Stop device status monitoring
    stopDeviceStatusMonitoring();
    
    // Stop receiver thread
    receiver_running_ = false;
    if (receiver_thread_.joinable()) {
        receiver_thread_.join();
    }
    
    // Close sockets
    closeSocket();
    closeMulticastSocket();
    
    BaseDeviceAdapter::stop();
}

bool EchonetLiteAdapter::startDiscovery(DeviceDiscoveryCallback callback, 
                                      std::chrono::milliseconds timeout_ms) {
    if (isDiscoveryInProgress()) {
        LOG_WARN("ECHONET Lite discovery already in progress");
        return false;
    }
    
    LOG_INFO("Starting ECHONET Lite device discovery (timeout: {} ms)", timeout_ms.count());
    
    discovery_callback_ = callback;
    discovery_timeout_ = timeout_ms;
    discovery_start_time_ = std::chrono::steady_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(discovered_devices_mutex_);
        discovered_devices_.clear();
    }
    
    // Start discovery thread
    discovery_running_ = true;
    discovery_thread_ = std::thread(&EchonetLiteAdapter::discoveryThreadFunc, this);
    
    return true;
}

void EchonetLiteAdapter::stopDiscovery() {
    if (!isDiscoveryInProgress()) {
        return;
    }
    
    LOG_INFO("Stopping ECHONET Lite device discovery");
    
    discovery_running_ = false;
    if (discovery_thread_.joinable()) {
        discovery_thread_.join();
    }
}

bool EchonetLiteAdapter::isDiscoveryInProgress() const {
    return discovery_running_;
}

ReadResult EchonetLiteAdapter::readRegister(const std::string& device_id, const RegisterAddress& address) {
    if (!isRunning()) {
        return createErrorReadResult("ECHONET Lite adapter not running");
    }
    
    if (!validateRegisterAddress(address)) {
        return createErrorReadResult("Invalid register address for ECHONET Lite");
    }
    
    auto device_info_opt = getDeviceInfo(device_id);
    if (!device_info_opt) {
        return createErrorReadResult("Device not found");
    }
    
    const auto& device_info = *device_info_opt;
    if (!device_info.online) {
        return createErrorReadResult("Device is offline");
    }
    
    // Create GET property frame
    std::vector<uint8_t> properties = {address.epc};
    EchonetLiteFrame frame = EchonetLiteFrame::createGetPropertyFrame(
        address.eoj_class_group_code, address.eoj_class_code, address.eoj_instance_code, properties);
    
    // Set transaction ID
    frame.tid = getNextTid();
    
    // Send request and wait for response
    auto response_opt = sendRequestWithResponse(device_id, frame);
    if (!response_opt) {
        return createErrorReadResult("No response from device");
    }
    
    const auto& response = *response_opt;
    if (!response.success) {
        return createErrorReadResult(response.error_message);
    }
    
    // Check if response is a GET response
    if (response.frame.esv != ESV_GET_RESPONSE && response.frame.esv != ESV_GET_SNA) {
        return createErrorReadResult("Unexpected response type");
    }
    
    // Check if response contains the requested property
    if (response.frame.properties.empty()) {
        return createErrorReadResult("No properties in response");
    }
    
    // Find the property in the response
    for (const auto& prop : response.frame.properties) {
        if (prop.epc == address.epc) {
            // Check if this is an error response
            if (response.frame.esv == ESV_GET_SNA) {
                return createErrorReadResult("Device reported property read not possible");
            }
            
            // Create result with property value
            RegisterValue value;
            value.type = DataType::BINARY;
            value.data = prop.edt;
            
            return createSuccessReadResult(value);
        }
    }
    
    return createErrorReadResult("Property not found in response");
}

WriteResult EchonetLiteAdapter::writeRegister(const std::string& device_id, const RegisterAddress& address, 
                                           const RegisterValue& value) {
    if (!isRunning()) {
        return createErrorWriteResult("ECHONET Lite adapter not running");
    }
    
    if (!validateRegisterAddress(address)) {
        return createErrorWriteResult("Invalid register address for ECHONET Lite");
    }
    
    if (!validateRegisterValue(address, value)) {
        return createErrorWriteResult("Invalid register value for ECHONET Lite");
    }
    
    auto device_info_opt = getDeviceInfo(device_id);
    if (!device_info_opt) {
        return createErrorWriteResult("Device not found");
    }
    
    const auto& device_info = *device_info_opt;
    if (!device_info.online) {
        return createErrorWriteResult("Device is offline");
    }
    
    // Create SET property frame
    EchonetLiteFrame frame = EchonetLiteFrame::createSetPropertyFrame(
        address.eoj_class_group_code, address.eoj_class_code, address.eoj_instance_code, 
        address.epc, value.data);
    
    // Set transaction ID
    frame.tid = getNextTid();
    
    // Send request and wait for response
    auto response_opt = sendRequestWithResponse(device_id, frame);
    if (!response_opt) {
        return createErrorWriteResult("No response from device");
    }
    
    const auto& response = *response_opt;
    if (!response.success) {
        return createErrorWriteResult(response.error_message);
    }
    
    // Check if response is a SET response
    if (response.frame.esv != ESV_SET_RESPONSE && response.frame.esv != ESV_SET_SNA) {
        return createErrorWriteResult("Unexpected response type");
    }
    
    // Check if response contains the requested property
    if (response.frame.properties.empty()) {
        return createErrorWriteResult("No properties in response");
    }
    
    // Find the property in the response
    for (const auto& prop : response.frame.properties) {
        if (prop.epc == address.epc) {
            // Check if this is an error response
            if (response.frame.esv == ESV_SET_SNA) {
                return createErrorWriteResult("Device reported property write not possible");
            }
            
            // Success
            return createSuccessWriteResult();
        }
    }
    
    return createErrorWriteResult("Property not found in response");
}

std::map<RegisterAddress, ReadResult> EchonetLiteAdapter::readMultipleRegisters(
    const std::string& device_id, const std::vector<RegisterAddress>& addresses) {
    
    std::map<RegisterAddress, ReadResult> results;
    
    if (!isRunning()) {
        for (const auto& address : addresses) {
            results[address] = createErrorReadResult("ECHONET Lite adapter not running");
        }
        return results;
    }
    
    auto device_info_opt = getDeviceInfo(device_id);
    if (!device_info_opt) {
        for (const auto& address : addresses) {
            results[address] = createErrorReadResult("Device not found");
        }
        return results;
    }
    
    const auto& device_info = *device_info_opt;
    if (!device_info.online) {
        for (const auto& address : addresses) {
            results[address] = createErrorReadResult("Device is offline");
        }
        return results;
    }
    
    // Group addresses by EOJ (ECHONET Lite object)
    std::map<std::tuple<uint8_t, uint8_t, uint8_t>, std::vector<RegisterAddress>> grouped_addresses;
    for (const auto& address : addresses) {
        if (!validateRegisterAddress(address)) {
            results[address] = createErrorReadResult("Invalid register address for ECHONET Lite");
            continue;
        }
        
        auto key = std::make_tuple(address.eoj_class_group_code, address.eoj_class_code, address.eoj_instance_code);
        grouped_addresses[key].push_back(address);
    }
    
    // Process each group
    for (const auto& group : grouped_addresses) {
        const auto& [eoj_key, group_addresses] = group;
        const auto& [class_group_code, class_code, instance_code] = eoj_key;
        
        // Extract EPCs
        std::vector<uint8_t> epcs;
        for (const auto& address : group_addresses) {
            epcs.push_back(address.epc);
        }
        
        // Create GET property frame
        EchonetLiteFrame frame = EchonetLiteFrame::createGetPropertyFrame(
            class_group_code, class_code, instance_code, epcs);
        
        // Set transaction ID
        frame.tid = getNextTid();
        
        // Send request and wait for response
        auto response_opt = sendRequestWithResponse(device_id, frame);
        if (!response_opt) {
            for (const auto& address : group_addresses) {
                results[address] = createErrorReadResult("No response from device");
            }
            continue;
        }
        
        const auto& response = *response_opt;
        if (!response.success) {
            for (const auto& address : group_addresses) {
                results[address] = createErrorReadResult(response.error_message);
            }
            continue;
        }
        
        // Check if response is a GET response
        if (response.frame.esv != ESV_GET_RESPONSE && response.frame.esv != ESV_GET_SNA) {
            for (const auto& address : group_addresses) {
                results[address] = createErrorReadResult("Unexpected response type");
            }
            continue;
        }
        
        // Process each property in the response
        std::set<uint8_t> processed_epcs;
        for (const auto& prop : response.frame.properties) {
            // Find the corresponding address
            // NOLINTNEXTLINE(cppcheck-useStlAlgorithm)
            for (const auto& address : group_addresses) {
                if (address.epc == prop.epc) {
                    // Check if this is an error response for this property
                    if (response.frame.esv == ESV_GET_SNA) {
                        results[address] = createErrorReadResult("Device reported property read not possible");
                    } else {
                        // Create result with property value
                        RegisterValue value;
                        value.type = DataType::BINARY;
                        value.data = prop.edt;
                        results[address] = createSuccessReadResult(value);
                    }
                    
                    processed_epcs.insert(prop.epc);
                    break;
                }
            }
        }
        
        // Handle properties not found in the response
        for (const auto& address : group_addresses) {
            if (processed_epcs.find(address.epc) == processed_epcs.end()) {
                results[address] = createErrorReadResult("Property not found in response");
            }
        }
    }
    
    return results;
}

bool EchonetLiteAdapter::validateDeviceAddress(const DeviceInfo& device_info) const {
    if (!BaseDeviceAdapter::validateDeviceAddress(device_info)) {
        return false;
    }
    
    // Check if address is an EchonetLiteAddress
    auto echonet_address = std::dynamic_pointer_cast<EchonetLiteAddress>(device_info.address);
    if (!echonet_address) {
        LOG_ERROR("Device address is not an ECHONET Lite address");
        return false;
    }
    
    // Check if IP address is valid
    if (echonet_address->ip_address.empty()) {
        LOG_ERROR("ECHONET Lite device address has empty IP address");
        return false;
    }
    
    return true;
}

bool EchonetLiteAdapter::validateRegisterAddress(const RegisterAddress& address) const {
    // Check if register type is EPC
    if (address.type != RegisterType::EPC) {
        LOG_ERROR("Register type must be EPC for ECHONET Lite");
        return false;
    }
    
    // Basic validation of EOJ and EPC
    if (address.eoj_class_group_code == 0 && address.eoj_class_code == 0) {
        LOG_ERROR("Invalid EOJ class codes");
        return false;
    }
    
    return true;
}

bool EchonetLiteAdapter::validateRegisterValue(const RegisterAddress& address, const RegisterValue& value) const {
    // Check if value is not empty
    if (value.data.empty()) {
        LOG_ERROR("Register value is empty");
        return false;
    }
    
    return true;
}

bool EchonetLiteAdapter::initializeSocket() {
    // Create UDP socket
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        LOG_ERROR("Failed to create socket: {}", strerror(errno));
        return false;
    }
    
    // Set socket options
    int reuse = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        LOG_ERROR("Failed to set SO_REUSEADDR: {}", strerror(errno));
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    // Set receive timeout
    struct timeval tv;
    tv.tv_sec = SOCKET_TIMEOUT_MS / 1000;
    tv.tv_usec = (SOCKET_TIMEOUT_MS % 1000) * 1000;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        LOG_ERROR("Failed to set SO_RCVTIMEO: {}", strerror(errno));
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    // Bind to ECHONET Lite port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(ECHONET_PORT);
    
    if (bind(socket_fd_, static_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind socket: {}", strerror(errno));
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    // Set non-blocking mode
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags < 0) {
        LOG_ERROR("Failed to get socket flags: {}", strerror(errno));
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    if (fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
        LOG_ERROR("Failed to set non-blocking mode: {}", strerror(errno));
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    LOG_INFO("ECHONET Lite socket initialized");
    return true;
}

bool EchonetLiteAdapter::initializeMulticastSocket() {
    // Create UDP socket for multicast
    multicast_socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (multicast_socket_fd_ < 0) {
        LOG_ERROR("Failed to create multicast socket: {}", strerror(errno));
        return false;
    }
    
    // Set socket options
    int reuse = 1;
    if (setsockopt(multicast_socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        LOG_ERROR("Failed to set SO_REUSEADDR for multicast: {}", strerror(errno));
        close(multicast_socket_fd_);
        multicast_socket_fd_ = -1;
        return false;
    }
    
    // Join multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(ECHONET_MULTICAST_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
    if (setsockopt(multicast_socket_fd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        LOG_ERROR("Failed to join multicast group: {}", strerror(errno));
        close(multicast_socket_fd_);
        multicast_socket_fd_ = -1;
        return false;
    }
    
    // Set multicast TTL
    int ttl = 1;
    if (setsockopt(multicast_socket_fd_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        LOG_ERROR("Failed to set multicast TTL: {}", strerror(errno));
        close(multicast_socket_fd_);
        multicast_socket_fd_ = -1;
        return false;
    }
    
    LOG_INFO("ECHONET Lite multicast socket initialized");
    return true;
}

void EchonetLiteAdapter::closeSocket() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

void EchonetLiteAdapter::closeMulticastSocket() {
    if (multicast_socket_fd_ >= 0) {
        // Leave multicast group
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = inet_addr(ECHONET_MULTICAST_ADDR);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        
        setsockopt(multicast_socket_fd_, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        
        close(multicast_socket_fd_);
        multicast_socket_fd_ = -1;
    }
}

void EchonetLiteAdapter::receiverThreadFunc() {
    LOG_INFO("ECHONET Lite receiver thread started");
    
    std::vector<uint8_t> buffer(MAX_UDP_PACKET_SIZE);
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    
    while (receiver_running_) {
        // Clear buffer
        std::fill(buffer.begin(), buffer.end(), 0);
        
        // Receive data
        ssize_t received = recvfrom(socket_fd_, buffer.data(), buffer.size(), 0,
                                   static_cast<struct sockaddr*>(&sender_addr), &sender_addr_len);
        
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available, sleep a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            LOG_ERROR("Error receiving data: {}", strerror(errno));
            continue;
        }
        
        if (received == 0) {
            // No data received
            continue;
        }
        
        // Get sender IP address
        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sender_addr.sin_addr), sender_ip, INET_ADDRSTRLEN);
        
        // Parse frame
        std::vector<uint8_t> frame_data(buffer.begin(), buffer.begin() + received);
        auto frame_opt = EchonetLiteFrame::deserialize(frame_data);
        
        if (!frame_opt) {
            LOG_WARN("Received invalid ECHONET Lite frame from {}", sender_ip);
            continue;
        }
        
        // Handle frame
        handleReceivedFrame(sender_ip, *frame_opt);
    }
    
    LOG_INFO("ECHONET Lite receiver thread stopped");
}

void EchonetLiteAdapter::discoveryThreadFunc() {
    LOG_INFO("ECHONET Lite discovery thread started");
    
    // Send discovery frame
    EchonetLiteFrame discovery_frame = EchonetLiteFrame::createDiscoveryFrame();
    discovery_frame.tid = getNextTid();
    
    if (!sendMulticastFrame(discovery_frame)) {
        LOG_ERROR("Failed to send discovery frame");
        discovery_running_ = false;
        return;
    }
    
    // Wait for responses until timeout
    while (discovery_running_) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - discovery_start_time_);
        
        if (elapsed >= discovery_timeout_) {
            LOG_INFO("ECHONET Lite discovery timeout reached");
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    discovery_running_ = false;
    LOG_INFO("ECHONET Lite discovery thread stopped");
}

bool EchonetLiteAdapter::sendFrame(const std::string& device_id, const EchonetLiteFrame& frame) {
    auto device_info_opt = getDeviceInfo(device_id);
    if (!device_info_opt) {
        LOG_ERROR("Device {} not found", device_id);
        return false;
    }
    
    const auto& device_info = *device_info_opt;
    auto echonet_address = std::dynamic_pointer_cast<EchonetLiteAddress>(device_info.address);
    if (!echonet_address) {
        LOG_ERROR("Device address is not an ECHONET Lite address");
        return false;
    }
    
    // Serialize frame
    std::vector<uint8_t> data = frame.serialize();
    
    // Set up destination address
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(echonet_address->port);
    
    if (inet_pton(AF_INET, echonet_address->ip_address.c_str(), &dest_addr.sin_addr) <= 0) {
        LOG_ERROR("Invalid IP address: {}", echonet_address->ip_address);
        return false;
    }
    
    // Send data
    ssize_t sent = sendto(socket_fd_, data.data(), data.size(), 0,
                         static_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
    
    if (sent < 0) {
        LOG_ERROR("Failed to send frame to {}: {}", echonet_address->ip_address, strerror(errno));
        return false;
    }
    
    if (static_cast<size_t>(sent) != data.size()) {
        LOG_WARN("Incomplete frame sent to {}: sent {} of {} bytes", 
                echonet_address->ip_address, sent, data.size());
        return false;
    }
    
    LOG_DEBUG("Sent ECHONET Lite frame to {}: TID={}, ESV=0x{:02X}, {} properties", 
             echonet_address->ip_address, frame.tid, frame.esv, frame.properties.size());
    
    return true;
}

bool EchonetLiteAdapter::sendMulticastFrame(const EchonetLiteFrame& frame) {
    // Serialize frame
    std::vector<uint8_t> data = frame.serialize();
    
    // Set up multicast address
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(ECHONET_PORT);
    
    if (inet_pton(AF_INET, ECHONET_MULTICAST_ADDR, &dest_addr.sin_addr) <= 0) {
        LOG_ERROR("Invalid multicast address: {}", ECHONET_MULTICAST_ADDR);
        return false;
    }
    
    // Send data
    ssize_t sent = sendto(multicast_socket_fd_, data.data(), data.size(), 0,
                         static_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
    
    if (sent < 0) {
        LOG_ERROR("Failed to send multicast frame: {}", strerror(errno));
        return false;
    }
    
    if (static_cast<size_t>(sent) != data.size()) {
        LOG_WARN("Incomplete multicast frame sent: sent {} of {} bytes", sent, data.size());
        return false;
    }
    
    LOG_DEBUG("Sent ECHONET Lite multicast frame: TID={}, ESV=0x{:02X}, {} properties", 
             frame.tid, frame.esv, frame.properties.size());
    
    return true;
}

std::optional<EchonetLiteResponse> EchonetLiteAdapter::sendRequestWithResponse(
    const std::string& device_id, const EchonetLiteFrame& frame, std::chrono::milliseconds timeout) {
    
    // Create response promise and future
    std::promise<EchonetLiteResponse> response_promise;
    std::future<EchonetLiteResponse> response_future = response_promise.get_future();
    
    // Register pending request
    {
        std::lock_guard<std::mutex> lock(pending_requests_mutex_);
        PendingRequest request;
        request.tid = frame.tid;
        request.timestamp = std::chrono::steady_clock::now();
        request.timeout = timeout;
        request.callback = [&response_promise](const EchonetLiteResponse& response) {
            response_promise.set_value(response);
        };
        
        pending_requests_[frame.tid] = request;
    }
    
    // Send frame
    if (!sendFrame(device_id, frame)) {
        // Remove pending request
        std::lock_guard<std::mutex> lock(pending_requests_mutex_);
        pending_requests_.erase(frame.tid);
        
        // Create error response
        EchonetLiteResponse error_response;
        error_response.tid = frame.tid;
        error_response.success = false;
        error_response.error_message = "Failed to send request";
        
        return error_response;
    }
    
    // Wait for response with timeout
    auto status = response_future.wait_for(timeout);
    
    // Remove pending request if still there
    {
        std::lock_guard<std::mutex> lock(pending_requests_mutex_);
        pending_requests_.erase(frame.tid);
    }
    
    if (status != std::future_status::ready) {
        // Try retransmission
        if (retransmitRequest(device_id, frame, frame.tid)) {
            // Wait for response again
            status = response_future.wait_for(timeout);
            
            if (status != std::future_status::ready) {
                LOG_WARN("No response received after retransmission for TID={}", frame.tid);
                return std::nullopt;
            }
        } else {
            LOG_WARN("No response received for TID={}", frame.tid);
            return std::nullopt;
        }
    }
    
    // Get response
    return response_future.get();
}

void EchonetLiteAdapter::handleReceivedFrame(const std::string& source_ip, const EchonetLiteFrame& frame) {
    LOG_DEBUG("Received ECHONET Lite frame from {}: TID={}, ESV=0x{:02X}, {} properties", 
             source_ip, frame.tid, frame.esv, frame.properties.size());
    
    // Check if this is a response to a discovery request
    if (isDiscoveryInProgress() && 
        frame.seoj_class_group_code == EOJ_NODE_PROFILE_CLASS_GROUP && 
        frame.seoj_class_code == EOJ_NODE_PROFILE_CLASS) {
        
        handleDiscoveryResponse(source_ip, frame);
        return;
    }
    
    // Check if this is a response to a pending request
    std::function<void(const EchonetLiteResponse&)> callback;
    
    {
        std::lock_guard<std::mutex> lock(pending_requests_mutex_);
        auto it = pending_requests_.find(frame.tid);
        
        if (it != pending_requests_.end()) {
            callback = it->second.callback;
        }
    }
    
    if (callback) {
        // Create response
        EchonetLiteResponse response;
        response.tid = frame.tid;
        response.frame = frame;
        response.success = true;
        
        // Call callback
        callback(response);
    }
}

void EchonetLiteAdapter::handleDiscoveryResponse(const std::string& source_ip, const EchonetLiteFrame& frame) {
    // Check if this is a response to a discovery request
    if (frame.esv != ESV_GET_RESPONSE || frame.properties.empty()) {
        return;
    }
    
    // Find instance list property
    for (const auto& prop : frame.properties) {
        if (prop.epc == EPC_SELF_NODE_INSTANCE_LIST_S) {
            // Parse instance list
            if (prop.edt.size() < 1) {
                LOG_WARN("Invalid instance list format from {}", source_ip);
                return;
            }
            
            uint8_t instance_count = prop.edt[0];
            if (prop.edt.size() < 1 + instance_count * 3) {
                LOG_WARN("Invalid instance list size from {}", source_ip);
                return;
            }
            
            // Process each instance
            for (uint8_t i = 0; i < instance_count; ++i) {
                size_t offset = 1 + i * 3;
                uint8_t class_group_code = prop.edt[offset];
                uint8_t class_code = prop.edt[offset + 1];
                uint8_t instance_code = prop.edt[offset + 2];
                
                // Skip node profile object
                if (class_group_code == EOJ_NODE_PROFILE_CLASS_GROUP && 
                    class_code == EOJ_NODE_PROFILE_CLASS) {
                    continue;
                }
                
                // Check if this is an EV charger or discharger (OCPP focused)
                if (class_group_code == EOJ_EV_CHARGER_CLASS_GROUP) {
                    std::string device_type;
                    std::string template_id;
                    
                    if (class_code == EOJ_EV_CHARGER_CLASS) {
                        device_type = "EV Charger";
                        template_id = "echonet_lite_charger";
                    } else if (class_code == EOJ_EV_DISCHARGER_CLASS) {
                        device_type = "EV Charger/Discharger";
                        template_id = "echonet_lite_charger_discharger";
                    } else {
                        // Skip other classes (storage battery, PV generation, etc.)
                        continue;
                    }
                    
                    // Generate device ID
                    std::string device_id = "echonet_" + source_ip + "_" + 
                                          std::to_string(class_group_code) + "_" + 
                                          std::to_string(class_code) + "_" + 
                                          std::to_string(instance_code);
                    
                    // Check if we've already discovered this device
                    {
                        std::lock_guard<std::mutex> lock(discovered_devices_mutex_);
                        if (discovered_devices_.find(device_id) != discovered_devices_.end()) {
                            continue;
                        }
                        discovered_devices_.insert(device_id);
                    }
                    
                    // Create device info
                    DeviceInfo device_info;
                    device_info.id = device_id;
                    device_info.name = "ECHONET Lite " + device_type;
                    device_info.model = "Unknown";
                    device_info.manufacturer = "Unknown";
                    device_info.protocol = DeviceProtocol::ECHONET_LITE;
                    device_info.template_id = template_id;
                    device_info.online = true;
                    device_info.last_seen = std::chrono::system_clock::now();
                    
                    // Create address
                    auto address = std::make_shared<EchonetLiteAddress>();
                    address->ip_address = source_ip;
                    address->port = ECHONET_PORT;
                    device_info.address = address;
                    
                    // Call discovery callback
                    if (discovery_callback_) {
                        discovery_callback_(device_info);
                    }
                    
                    LOG_INFO("Discovered ECHONET Lite {}: {} at {}", device_type, device_id, source_ip);
                }
            }
            
            break;
        }
    }
}

uint16_t EchonetLiteAdapter::getNextTid() {
    // Get next transaction ID (1-65535)
    uint16_t tid = next_tid_++;
    if (tid == 0) {
        tid = next_tid_++;
    }
    return tid;
}

void EchonetLiteAdapter::startDeviceStatusMonitoring() {
    status_monitor_running_ = true;
    status_monitor_thread_ = std::thread(&EchonetLiteAdapter::statusMonitorThreadFunc, this);
}

void EchonetLiteAdapter::stopDeviceStatusMonitoring() {
    status_monitor_running_ = false;
    if (status_monitor_thread_.joinable()) {
        status_monitor_thread_.join();
    }
}

void EchonetLiteAdapter::statusMonitorThreadFunc() {
    LOG_INFO("ECHONET Lite status monitor thread started");
    
    while (status_monitor_running_) {
        // Get all devices
        auto devices = getAllDevices();
        
        for (const auto& device : devices) {
            // Skip offline devices
            if (!device.online) {
                continue;
            }
            
            // Create operation status request for different device types
            RegisterAddress address;
            address.type = RegisterType::EPC;
            address.eoj_class_group_code = EOJ_EV_CHARGER_CLASS_GROUP;
            address.eoj_instance_code = 0x01;
            address.epc = EPC_OPERATION_STATUS;
            
            // Try different device classes (OCPP focused)
            std::vector<uint8_t> device_classes = {
                EOJ_EV_CHARGER_CLASS,
                EOJ_EV_DISCHARGER_CLASS
            };
            
            bool device_online = false;
            for (uint8_t class_code : device_classes) {
                address.eoj_class_code = class_code;
                
                // Read operation status
                auto result = readRegister(device.id, address);
                if (result.success) {
                    device_online = true;
                    break;
                }
            }
            
            // Update device status
            updateDeviceStatus(device.id, device_online);
        }
        
        // Sleep for monitoring interval
        std::this_thread::sleep_for(std::chrono::milliseconds(STATUS_MONITOR_INTERVAL_MS));
    }
    
    LOG_INFO("ECHONET Lite status monitor thread stopped");
}

bool EchonetLiteAdapter::retransmitRequest(const std::string& device_id, const EchonetLiteFrame& frame, 
                                         uint16_t tid, int max_retries) {
    for (int retry = 0; retry < max_retries; ++retry) {
        LOG_DEBUG("Retransmitting request to {} (TID={}, retry {}/{})", 
                 device_id, tid, retry + 1, max_retries);
        
        if (sendFrame(device_id, frame)) {
            return true;
        }
        
        // Wait before retrying
        std::this_thread::sleep_for(std::chrono::milliseconds(100 * (retry + 1)));
    }
    
    return false;
}

} // namespace device
} // namespace ocpp_gateway