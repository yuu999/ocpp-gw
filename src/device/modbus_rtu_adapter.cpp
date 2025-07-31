#include "ocpp_gateway/device/modbus_rtu_adapter.h"
#ifdef MODBUS_SUPPORT_ENABLED
#include <modbus/modbus.h>
#endif
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <random>

namespace ocpp_gateway {
namespace device {

#ifdef MODBUS_SUPPORT_ENABLED

// Modbus function codes
// Duplicate constant definitions removed (they already exist in modbus.h)
// static const int MODBUS_FC_READ_COILS = 0x01;
// static const int MODBUS_FC_READ_DISCRETE_INPUTS = 0x02;
// static const int MODBUS_FC_READ_HOLDING_REGISTERS = 0x03;
// static const int MODBUS_FC_READ_INPUT_REGISTERS = 0x04;
// static const int MODBUS_FC_WRITE_SINGLE_COIL = 0x05;
// static const int MODBUS_FC_WRITE_SINGLE_REGISTER = 0x06;
// static const int MODBUS_FC_WRITE_MULTIPLE_COILS = 0x0F;
// static const int MODBUS_FC_WRITE_MULTIPLE_REGISTERS = 0x10;

// Modbus register limits
// static const int MODBUS_MAX_READ_BITS = 2000;
// static const int MODBUS_MAX_WRITE_BITS = 1968;
// static const int MODBUS_MAX_READ_REGISTERS = 125;
// static const int MODBUS_MAX_WRITE_REGISTERS = 123;

// Modbus timeout in milliseconds
static const int MODBUS_TIMEOUT_MS = 1000;

// Status monitoring interval in milliseconds
static const int STATUS_MONITOR_INTERVAL_MS = 30000;

// Helper function to convert parity string to modbus parity char
char parityToModbusChar(const std::string& parity) {
    if (parity == "N" || parity == "n") {
        return 'N';
    } else if (parity == "E" || parity == "e") {
        return 'E';
    } else if (parity == "O" || parity == "o") {
        return 'O';
    }
    return 'N'; // Default to no parity
}

ModbusRtuAdapter::ModbusRtuAdapter()
    : BaseDeviceAdapter(DeviceProtocol::MODBUS_RTU) {
}

ModbusRtuAdapter::~ModbusRtuAdapter() {
    stop();
}

bool ModbusRtuAdapter::initialize() {
    if (!BaseDeviceAdapter::initialize()) {
        return false;
    }
    
    LOG_INFO("Initializing Modbus RTU adapter");
    return true;
}

bool ModbusRtuAdapter::start() {
    if (!BaseDeviceAdapter::start()) {
        return false;
    }
    
    LOG_INFO("Starting Modbus RTU adapter");
    
    // Start polling thread
    polling_running_ = true;
    polling_thread_ = std::thread(&ModbusRtuAdapter::pollingThreadFunc, this);
    
    // Start device status monitoring
    startDeviceStatusMonitoring();
    
    return true;
}

void ModbusRtuAdapter::stop() {
    if (!isRunning()) {
        return;
    }
    
    LOG_INFO("Stopping Modbus RTU adapter");
    
    // Stop discovery if in progress
    stopDiscovery();
    
    // Stop device status monitoring
    stopDeviceStatusMonitoring();
    
    // Stop polling thread
    polling_running_ = false;
    if (polling_thread_.joinable()) {
        polling_thread_.join();
    }
    
    // Close all connections
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& conn_pair : connections_) {
            closeConnection(conn_pair.second);
        }
        connections_.clear();
    }
    
    BaseDeviceAdapter::stop();
}

bool ModbusRtuAdapter::startDiscovery(DeviceDiscoveryCallback callback, 
                                    std::chrono::milliseconds timeout_ms) {
    if (isDiscoveryInProgress()) {
        LOG_WARN("Modbus RTU discovery already in progress");
        return false;
    }
    
    LOG_INFO("Starting Modbus RTU device discovery (timeout: {} ms)", timeout_ms.count());
    
    discovery_callback_ = callback;
    discovery_timeout_ = timeout_ms;
    discovery_start_time_ = std::chrono::steady_clock::now();
    
    // Start discovery thread
    discovery_running_ = true;
    discovery_thread_ = std::thread(&ModbusRtuAdapter::discoveryThreadFunc, this);
    
    return true;
}

void ModbusRtuAdapter::stopDiscovery() {
    if (!isDiscoveryInProgress()) {
        return;
    }
    
    LOG_INFO("Stopping Modbus RTU device discovery");
    
    discovery_running_ = false;
    if (discovery_thread_.joinable()) {
        discovery_thread_.join();
    }
}

bool ModbusRtuAdapter::isDiscoveryInProgress() const {
    return discovery_running_;
}

ReadResult ModbusRtuAdapter::readRegister(const std::string& device_id, const RegisterAddress& address) {
    if (!isRunning()) {
        return createErrorReadResult("Modbus RTU adapter not running");
    }
    
    if (!validateRegisterAddress(address)) {
        return createErrorReadResult("Invalid register address for Modbus RTU");
    }
    
    auto device_info_opt = getDeviceInfo(device_id);
    if (!device_info_opt) {
        return createErrorReadResult("Device not found");
    }
    
    const auto& device_info = *device_info_opt;
    if (!device_info.online) {
        return createErrorReadResult("Device is offline");
    }
    
    // Get Modbus connection
    auto connection = getConnection(device_id);
    if (!connection || !connection->connected) {
        return createErrorReadResult("Failed to connect to device");
    }
    
    // Get unit ID
    auto modbus_address = std::dynamic_pointer_cast<ModbusRtuAddress>(device_info.address);
    if (!modbus_address) {
        return createErrorReadResult("Invalid device address");
    }
    
    int unit_id = modbus_address->unit_id;
    
    // Lock the connection for thread safety
    std::lock_guard<std::mutex> lock(connection->mutex);
    
    // Set the slave ID
    modbus_set_slave(connection->ctx, unit_id);
    
    // Read the register based on its type
    switch (address.type) {
        case RegisterType::COIL:
            return readCoil(connection, unit_id, address);
        
        case RegisterType::DISCRETE_INPUT:
            return readDiscreteInput(connection, unit_id, address);
        
        case RegisterType::INPUT_REGISTER:
            return readInputRegister(connection, unit_id, address);
        
        case RegisterType::HOLDING_REGISTER:
            return readHoldingRegister(connection, unit_id, address);
        
        default:
            return createErrorReadResult("Unsupported register type");
    }
}

WriteResult ModbusRtuAdapter::writeRegister(const std::string& device_id, const RegisterAddress& address, 
                                          const RegisterValue& value) {
    if (!isRunning()) {
        return createErrorWriteResult("Modbus RTU adapter not running");
    }
    
    if (!validateRegisterAddress(address)) {
        return createErrorWriteResult("Invalid register address for Modbus RTU");
    }
    
    if (!validateRegisterValue(address, value)) {
        return createErrorWriteResult("Invalid register value for Modbus RTU");
    }
    
    auto device_info_opt = getDeviceInfo(device_id);
    if (!device_info_opt) {
        return createErrorWriteResult("Device not found");
    }
    
    const auto& device_info = *device_info_opt;
    if (!device_info.online) {
        return createErrorWriteResult("Device is offline");
    }
    
    // Get Modbus connection
    auto connection = getConnection(device_id);
    if (!connection || !connection->connected) {
        return createErrorWriteResult("Failed to connect to device");
    }
    
    // Get unit ID
    auto modbus_address = std::dynamic_pointer_cast<ModbusRtuAddress>(device_info.address);
    if (!modbus_address) {
        return createErrorWriteResult("Invalid device address");
    }
    
    int unit_id = modbus_address->unit_id;
    
    // Lock the connection for thread safety
    std::lock_guard<std::mutex> lock(connection->mutex);
    
    // Set the slave ID
    modbus_set_slave(connection->ctx, unit_id);
    
    // Write the register based on its type
    switch (address.type) {
        case RegisterType::COIL:
            return writeCoil(connection, unit_id, address, value);
        
        case RegisterType::HOLDING_REGISTER:
            return writeHoldingRegister(connection, unit_id, address, value);
        
        case RegisterType::DISCRETE_INPUT:
        case RegisterType::INPUT_REGISTER:
            return createErrorWriteResult("Cannot write to read-only register type");
        
        default:
            return createErrorWriteResult("Unsupported register type");
    }
}

std::map<RegisterAddress, ReadResult> ModbusRtuAdapter::readMultipleRegisters(
    const std::string& device_id, const std::vector<RegisterAddress>& addresses) {
    
    std::map<RegisterAddress, ReadResult> results;
    
    if (!isRunning()) {
        for (const auto& address : addresses) {
            results[address] = createErrorReadResult("Modbus RTU adapter not running");
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
    
    // Get Modbus connection
    auto connection = getConnection(device_id);
    if (!connection || !connection->connected) {
        for (const auto& address : addresses) {
            results[address] = createErrorReadResult("Failed to connect to device");
        }
        return results;
    }
    
    // Get unit ID
    auto modbus_address = std::dynamic_pointer_cast<ModbusRtuAddress>(device_info.address);
    if (!modbus_address) {
        for (const auto& address : addresses) {
            results[address] = createErrorReadResult("Invalid device address");
        }
        return results;
    }
    
    int unit_id = modbus_address->unit_id;
    
    // Validate all addresses
    std::vector<RegisterAddress> valid_addresses;
    for (const auto& address : addresses) {
        if (validateRegisterAddress(address)) {
            valid_addresses.push_back(address);
        } else {
            results[address] = createErrorReadResult("Invalid register address for Modbus RTU");
        }
    }
    
    // Group registers by type and contiguous addresses
    auto groups = groupRegisters(valid_addresses);
    
    // Lock the connection for thread safety
    std::lock_guard<std::mutex> lock(connection->mutex);
    
    // Set the slave ID
    modbus_set_slave(connection->ctx, unit_id);
    
    // Process each group
    for (const auto& group : groups) {
        switch (group.type) {
            case RegisterType::COIL: {
                // Read coils
                uint8_t coil_values[MODBUS_MAX_READ_BITS] = {0};
                int rc = modbus_read_bits(connection->ctx, group.start_address, group.count, coil_values);
                
                if (rc == -1) {
                    // Error reading coils
                    for (const auto& addr : group.addresses) {
                        results[addr] = handleModbusError("read coils", errno);
                    }
                } else {
                    // Process each address in the group
                    for (const auto& addr : group.addresses) {
                        uint32_t offset = addr.address - group.start_address;
                        RegisterValue value;
                        value.type = DataType::BOOL;
                        value.setBool(coil_values[offset] != 0);
                        results[addr] = createSuccessReadResult(value);
                    }
                }
                break;
            }
            
            case RegisterType::DISCRETE_INPUT: {
                // Read discrete inputs
                uint8_t input_values[MODBUS_MAX_READ_BITS] = {0};
                int rc = modbus_read_input_bits(connection->ctx, group.start_address, group.count, input_values);
                
                if (rc == -1) {
                    // Error reading discrete inputs
                    for (const auto& addr : group.addresses) {
                        results[addr] = handleModbusError("read discrete inputs", errno);
                    }
                } else {
                    // Process each address in the group
                    for (const auto& addr : group.addresses) {
                        uint32_t offset = addr.address - group.start_address;
                        RegisterValue value;
                        value.type = DataType::BOOL;
                        value.setBool(input_values[offset] != 0);
                        results[addr] = createSuccessReadResult(value);
                    }
                }
                break;
            }
            
            case RegisterType::INPUT_REGISTER: {
                // Read input registers
                uint16_t input_regs[MODBUS_MAX_READ_REGISTERS] = {0};
                int rc = modbus_read_input_registers(connection->ctx, group.start_address, group.count, input_regs);
                
                if (rc == -1) {
                    // Error reading input registers
                    for (const auto& addr : group.addresses) {
                        results[addr] = handleModbusError("read input registers", errno);
                    }
                } else {
                    // Process each address in the group
                    for (const auto& addr : group.addresses) {
                        uint32_t offset = addr.address - group.start_address;
                        uint16_t* reg_ptr = &input_regs[offset];
                        
                        // Determine data type based on count
                        DataType data_type = DataType::UINT16;
                        if (addr.count == 2) {
                            data_type = DataType::UINT32;
                        } else if (addr.count == 4) {
                            data_type = DataType::UINT64;
                        }
                        
                        RegisterValue value = convertToRegisterValue(reg_ptr, addr.count, data_type);
                        results[addr] = createSuccessReadResult(value);
                    }
                }
                break;
            }
            
            case RegisterType::HOLDING_REGISTER: {
                // Read holding registers
                uint16_t holding_regs[MODBUS_MAX_READ_REGISTERS] = {0};
                int rc = modbus_read_registers(connection->ctx, group.start_address, group.count, holding_regs);
                
                if (rc == -1) {
                    // Error reading holding registers
                    for (const auto& addr : group.addresses) {
                        results[addr] = handleModbusError("read holding registers", errno);
                    }
                } else {
                    // Process each address in the group
                    for (const auto& addr : group.addresses) {
                        uint32_t offset = addr.address - group.start_address;
                        uint16_t* reg_ptr = &holding_regs[offset];
                        
                        // Determine data type based on count
                        DataType data_type = DataType::UINT16;
                        if (addr.count == 2) {
                            data_type = DataType::UINT32;
                        } else if (addr.count == 4) {
                            data_type = DataType::UINT64;
                        }
                        
                        RegisterValue value = convertToRegisterValue(reg_ptr, addr.count, data_type);
                        results[addr] = createSuccessReadResult(value);
                    }
                }
                break;
            }
            
            default:
                // Unsupported register type
                for (const auto& addr : group.addresses) {
                    results[addr] = createErrorReadResult("Unsupported register type");
                }
                break;
        }
    }
    
    return results;
}

std::map<RegisterAddress, WriteResult> ModbusRtuAdapter::writeMultipleRegisters(
    const std::string& device_id, const std::map<RegisterAddress, RegisterValue>& values) {
    
    std::map<RegisterAddress, WriteResult> results;
    
    if (!isRunning()) {
        for (const auto& value_pair : values) {
            results[value_pair.first] = createErrorWriteResult("Modbus RTU adapter not running");
        }
        return results;
    }
    
    auto device_info_opt = getDeviceInfo(device_id);
    if (!device_info_opt) {
        for (const auto& value_pair : values) {
            results[value_pair.first] = createErrorWriteResult("Device not found");
        }
        return results;
    }
    
    const auto& device_info = *device_info_opt;
    if (!device_info.online) {
        for (const auto& value_pair : values) {
            results[value_pair.first] = createErrorWriteResult("Device is offline");
        }
        return results;
    }
    
    // Get Modbus connection
    auto connection = getConnection(device_id);
    if (!connection || !connection->connected) {
        for (const auto& value_pair : values) {
            results[value_pair.first] = createErrorWriteResult("Failed to connect to device");
        }
        return results;
    }
    
    // Get unit ID
    auto modbus_address = std::dynamic_pointer_cast<ModbusRtuAddress>(device_info.address);
    if (!modbus_address) {
        for (const auto& value_pair : values) {
            results[value_pair.first] = createErrorWriteResult("Invalid device address");
        }
        return results;
    }
    
    int unit_id = modbus_address->unit_id;
    
    // Lock the connection for thread safety
    std::lock_guard<std::mutex> lock(connection->mutex);
    
    // Set the slave ID
    modbus_set_slave(connection->ctx, unit_id);
    
    // Process each register individually
    for (const auto& value_pair : values) {
        const auto& address = value_pair.first;
        const auto& value = value_pair.second;
        
        if (!validateRegisterAddress(address)) {
            results[address] = createErrorWriteResult("Invalid register address for Modbus RTU");
            continue;
        }
        
        if (!validateRegisterValue(address, value)) {
            results[address] = createErrorWriteResult("Invalid register value for Modbus RTU");
            continue;
        }
        
        // Write the register based on its type
        switch (address.type) {
            case RegisterType::COIL:
                results[address] = writeCoil(connection, unit_id, address, value);
                break;
            
            case RegisterType::HOLDING_REGISTER:
                results[address] = writeHoldingRegister(connection, unit_id, address, value);
                break;
            
            case RegisterType::DISCRETE_INPUT:
            case RegisterType::INPUT_REGISTER:
                results[address] = createErrorWriteResult("Cannot write to read-only register type");
                break;
            
            default:
                results[address] = createErrorWriteResult("Unsupported register type");
                break;
        }
    }
    
    return results;
}

bool ModbusRtuAdapter::validateDeviceAddress(const DeviceInfo& device_info) const {
    if (!BaseDeviceAdapter::validateDeviceAddress(device_info)) {
        return false;
    }
    
    // Check if address is a ModbusRtuAddress
    auto modbus_address = std::dynamic_pointer_cast<ModbusRtuAddress>(device_info.address);
    if (!modbus_address) {
        LOG_ERROR("Device address is not a Modbus RTU address");
        return false;
    }
    
    // Check if serial port is valid
    if (modbus_address->port.empty()) {
        LOG_ERROR("Modbus RTU device address has empty serial port");
        return false;
    }
    
    // Check if unit ID is valid (1-247)
    if (modbus_address->unit_id < 1 || modbus_address->unit_id > 247) {
        LOG_ERROR("Invalid Modbus RTU unit ID: {}", modbus_address->unit_id);
        return false;
    }
    
    return true;
}

bool ModbusRtuAdapter::validateRegisterAddress(const RegisterAddress& address) const {
    // Check if register type is valid for Modbus
    if (address.type != RegisterType::COIL &&
        address.type != RegisterType::DISCRETE_INPUT &&
        address.type != RegisterType::INPUT_REGISTER &&
        address.type != RegisterType::HOLDING_REGISTER) {
        LOG_ERROR("Invalid register type for Modbus RTU");
        return false;
    }
    
    // Check if address is valid (0-65535)
    if (address.address > 65535) {
        LOG_ERROR("Invalid Modbus register address: {}", address.address);
        return false;
    }
    
    // Check if count is valid
    if (address.count < 1) {
        LOG_ERROR("Invalid register count: {}", address.count);
        return false;
    }
    
    // Check if count is valid for the register type
    if (address.type == RegisterType::COIL || address.type == RegisterType::DISCRETE_INPUT) {
        if (address.count > MODBUS_MAX_READ_BITS) {
            LOG_ERROR("Invalid bit count: {}", address.count);
            return false;
        }
    } else {
        if (address.count > MODBUS_MAX_READ_REGISTERS) {
            LOG_ERROR("Invalid register count: {}", address.count);
            return false;
        }
    }
    
    return true;
}

bool ModbusRtuAdapter::validateRegisterValue(const RegisterAddress& address, const RegisterValue& value) const {
    // Check if value is not empty
    if (value.data.empty()) {
        LOG_ERROR("Register value is empty");
        return false;
    }
    
    // Check if value size is valid for the register type
    if (address.type == RegisterType::COIL) {
        // For coils, we only need 1 byte
        if (value.data.size() != 1) {
            LOG_ERROR("Invalid value size for coil: {}", value.data.size());
            return false;
        }
    } else if (address.type == RegisterType::HOLDING_REGISTER) {
        // For holding registers, we need 2 bytes per register
        if (value.data.size() < 2 || value.data.size() > address.count * 2) {
            LOG_ERROR("Invalid value size for holding register: {}", value.data.size());
            return false;
        }
    }
    
    return true;
}

std::shared_ptr<ModbusRtuAdapter::ModbusConnection> ModbusRtuAdapter::getConnection(const std::string& device_id) {
    auto device_info_opt = getDeviceInfo(device_id);
    if (!device_info_opt) {
        LOG_ERROR("Device {} not found", device_id);
        return nullptr;
    }
    
    const auto& device_info = *device_info_opt;
    auto modbus_address = std::dynamic_pointer_cast<ModbusRtuAddress>(device_info.address);
    if (!modbus_address) {
        LOG_ERROR("Device address is not a Modbus RTU address");
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    // Check if we already have a connection for this port
    auto it = connections_.find(modbus_address->port);
    if (it != connections_.end()) {
        // Update last used time
        it->second->last_used = std::chrono::steady_clock::now();
        
        // Add unit ID to the set if not already present
        it->second->unit_ids.insert(modbus_address->unit_id);
        
        return it->second;
    }
    
    // Create a new connection
    auto connection = createConnection(*modbus_address);
    if (connection) {
        connections_[modbus_address->port] = connection;
        connection->unit_ids.insert(modbus_address->unit_id);
    }
    
    return connection;
}

std::shared_ptr<ModbusRtuAdapter::ModbusConnection> ModbusRtuAdapter::createConnection(const ModbusRtuAddress& address) {
    LOG_INFO("Creating Modbus RTU connection to {}", address.port);
    
    auto connection = std::make_shared<ModbusConnection>();
    connection->port = address.port;
    connection->baud_rate = address.baud_rate;
    connection->data_bits = address.data_bits;
    connection->stop_bits = address.stop_bits;
    connection->parity = address.parity;
    connection->last_used = std::chrono::steady_clock::now();
    connection->connected = false;
    
    // Create Modbus context
    connection->ctx = modbus_new_rtu(
        address.port.c_str(),
        address.baud_rate,
        parityToModbusChar(address.parity),
        address.data_bits,
        address.stop_bits
    );
    
    if (!connection->ctx) {
        LOG_ERROR("Failed to create Modbus RTU context: {}", modbus_strerror(errno));
        return nullptr;
    }
    
    // Set response timeout
    modbus_set_response_timeout(connection->ctx, MODBUS_TIMEOUT_MS / 1000, (MODBUS_TIMEOUT_MS % 1000) * 1000);
    
    // Connect to the device
    if (modbus_connect(connection->ctx) == -1) {
        LOG_ERROR("Failed to connect to Modbus RTU device on {}: {}", address.port, modbus_strerror(errno));
        modbus_free(connection->ctx);
        return nullptr;
    }
    
    connection->connected = true;
    LOG_INFO("Connected to Modbus RTU device on {}", address.port);
    
    return connection;
}

void ModbusRtuAdapter::closeConnection(std::shared_ptr<ModbusConnection> connection) {
    if (!connection) {
        return;
    }
    
    if (connection->connected) {
        modbus_close(connection->ctx);
        connection->connected = false;
    }
    
    if (connection->ctx) {
        modbus_free(connection->ctx);
        connection->ctx = nullptr;
    }
    
    LOG_INFO("Closed Modbus RTU connection to {}", connection->port);
}

void ModbusRtuAdapter::pollingThreadFunc() {
    LOG_INFO("Modbus RTU polling thread started");
    
    while (polling_running_) {
        // Sleep for a short time to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Get all devices
        auto devices = getAllDevices();
        
        // Poll each device
        for (const auto& device : devices) {
            // Skip offline devices
            if (!device.online) {
                continue;
            }
            
            // Get polling configuration for this device
            PollingConfig config;
            {
                std::lock_guard<std::mutex> lock(polling_configs_mutex_);
                auto it = polling_configs_.find(device.id);
                if (it != polling_configs_.end()) {
                    config = it->second;
                } else {
                    // No polling configuration for this device
                    continue;
                }
            }
            
            // Skip if no addresses to poll
            if (config.addresses.empty()) {
                continue;
            }
            
            // Read registers
            auto results = readMultipleRegisters(device.id, config.addresses);
            
            // Process results (e.g., update device status)
            bool device_online = false;
            for (const auto& result : results) {
                if (result.second.success) {
                    device_online = true;
                    break;
                }
            }
            
            // Update device status
            updateDeviceStatus(device.id, device_online);
        }
    }
    
    LOG_INFO("Modbus RTU polling thread stopped");
}

void ModbusRtuAdapter::discoveryThreadFunc() {
    LOG_INFO("Modbus RTU discovery thread started");
    
    // List of common serial ports to check
    std::vector<std::string> serial_ports = {
        "/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3",
        "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2", "/dev/ttyUSB3",
        "/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyACM2", "/dev/ttyACM3"
    };
    
    // Common baud rates to try
    std::vector<int> baud_rates = {9600, 19200, 38400, 57600, 115200};
    
    // Common parity settings
    std::vector<std::string> parities = {"N", "E", "O"};
    
    // Discover devices on each serial port
    for (const auto& port : serial_ports) {
        if (!discovery_running_) {
            break;
        }
        
        // Check if we can open the port
        int fd = open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) {
            // Port doesn't exist or can't be opened
            continue;
        }
        
        // Close the port
        close(fd);
        
        LOG_INFO("Checking for Modbus RTU devices on {}", port);
        
        // Try different baud rates
        for (int baud_rate : baud_rates) {
            if (!discovery_running_) {
                break;
            }
            
            // Try different parity settings
            for (const auto& parity : parities) {
                if (!discovery_running_) {
                    break;
                }
                
                // Create Modbus context
                modbus_t* ctx = modbus_new_rtu(port.c_str(), baud_rate, parity[0], 8, 1);
                if (!ctx) {
                    continue;
                }
                
                // Set response timeout (shorter for discovery)
                modbus_set_response_timeout(ctx, 0, 500000); // 500ms
                
                // Try to connect
                if (modbus_connect(ctx) == -1) {
                    modbus_free(ctx);
                    continue;
                }
                
                // Try different unit IDs
                for (int unit_id = 1; unit_id <= 247; unit_id++) {
                    if (!discovery_running_) {
                        break;
                    }
                    
                    // Set the slave ID
                    modbus_set_slave(ctx, unit_id);
                    
                    // Try to read a holding register (common register type)
                    uint16_t reg_value;
                    int rc = modbus_read_registers(ctx, 0, 1, &reg_value);
                    
                    if (rc == 1) {
                        // Device found!
                        LOG_INFO("Found Modbus RTU device on {} (baud: {}, parity: {}, unit: {})",
                                port, baud_rate, parity, unit_id);
                        
                        // Create device info
                        DeviceInfo device_info;
                        device_info.id = "modbus_rtu_" + port + "_" + std::to_string(unit_id);
                        device_info.name = "Modbus RTU Device";
                        device_info.model = "Unknown";
                        device_info.manufacturer = "Unknown";
                        device_info.protocol = DeviceProtocol::MODBUS_RTU;
                        
                        // Create address
                        auto address = std::make_shared<ModbusRtuAddress>();
                        address->port = port;
                        address->baud_rate = baud_rate;
                        address->data_bits = 8;
                        address->stop_bits = 1;
                        address->parity = parity;
                        address->unit_id = unit_id;
                        device_info.address = address;
                        
                        // Call discovery callback
                        if (discovery_callback_) {
                            discovery_callback_(device_info);
                        }
                    }
                }
                
                // Close and free the context
                modbus_close(ctx);
                modbus_free(ctx);
            }
        }
    }
    
    discovery_running_ = false;
    LOG_INFO("Modbus RTU discovery thread stopped");
}

ReadResult ModbusRtuAdapter::readCoil(std::shared_ptr<ModbusConnection> connection, int unit_id, const RegisterAddress& address) {
    uint8_t coil_value;
    int rc = modbus_read_bits(connection->ctx, address.address, 1, &coil_value);
    
    if (rc == -1) {
        return handleModbusError("read coil", errno);
    }
    
    RegisterValue value;
    value.type = DataType::BOOL;
    value.setBool(coil_value != 0);
    
    return createSuccessReadResult(value);
}

ReadResult ModbusRtuAdapter::readDiscreteInput(std::shared_ptr<ModbusConnection> connection, int unit_id, const RegisterAddress& address) {
    uint8_t input_value;
    int rc = modbus_read_input_bits(connection->ctx, address.address, 1, &input_value);
    
    if (rc == -1) {
        return handleModbusError("read discrete input", errno);
    }
    
    RegisterValue value;
    value.type = DataType::BOOL;
    value.setBool(input_value != 0);
    
    return createSuccessReadResult(value);
}

ReadResult ModbusRtuAdapter::readInputRegister(std::shared_ptr<ModbusConnection> connection, int unit_id, const RegisterAddress& address) {
    uint16_t reg_values[MODBUS_MAX_READ_REGISTERS];
    int rc = modbus_read_input_registers(connection->ctx, address.address, address.count, reg_values);
    
    if (rc == -1) {
        return handleModbusError("read input register", errno);
    }
    
    // Determine data type based on count
    DataType data_type = DataType::UINT16;
    if (address.count == 2) {
        data_type = DataType::UINT32;
    } else if (address.count == 4) {
        data_type = DataType::UINT64;
    }
    
    RegisterValue value = convertToRegisterValue(reg_values, address.count, data_type);
    return createSuccessReadResult(value);
}

ReadResult ModbusRtuAdapter::readHoldingRegister(std::shared_ptr<ModbusConnection> connection, int unit_id, const RegisterAddress& address) {
    uint16_t reg_values[MODBUS_MAX_READ_REGISTERS];
    int rc = modbus_read_registers(connection->ctx, address.address, address.count, reg_values);
    
    if (rc == -1) {
        return handleModbusError("read holding register", errno);
    }
    
    // Determine data type based on count
    DataType data_type = DataType::UINT16;
    if (address.count == 2) {
        data_type = DataType::UINT32;
    } else if (address.count == 4) {
        data_type = DataType::UINT64;
    }
    
    RegisterValue value = convertToRegisterValue(reg_values, address.count, data_type);
    return createSuccessReadResult(value);
}

WriteResult ModbusRtuAdapter::writeCoil(std::shared_ptr<ModbusConnection> connection, int unit_id, const RegisterAddress& address, const RegisterValue& value) {
    int rc;
    
    if (address.count == 1) {
        // Write single coil
        rc = modbus_write_bit(connection->ctx, address.address, value.getBool() ? 1 : 0);
    } else {
        // Write multiple coils
        uint8_t coil_values[MODBUS_MAX_WRITE_BITS] = {0};
        for (size_t i = 0; i < value.data.size() && i < address.count; ++i) {
            coil_values[i] = (value.data[i] != 0) ? 1 : 0;
        }
        
        rc = modbus_write_bits(connection->ctx, address.address, address.count, coil_values);
    }
    
    if (rc == -1) {
        return handleModbusWriteError("write coil", errno);
    }
    
    return createSuccessWriteResult();
}

WriteResult ModbusRtuAdapter::writeHoldingRegister(std::shared_ptr<ModbusConnection> connection, int unit_id, const RegisterAddress& address, const RegisterValue& value) {
    int rc;
    
    if (address.count == 1) {
        // Write single register
        uint16_t reg_value = value.getUint16();
        rc = modbus_write_register(connection->ctx, address.address, reg_value);
    } else {
        // Write multiple registers
        size_t count;
        std::vector<uint16_t> reg_values = convertFromRegisterValue(value, count);
        
        if (count > address.count) {
            count = address.count;
        }
        
        rc = modbus_write_registers(connection->ctx, address.address, count, reg_values.data());
    }
    
    if (rc == -1) {
        return handleModbusWriteError("write holding register", errno);
    }
    
    return createSuccessWriteResult();
}

RegisterValue ModbusRtuAdapter::convertToRegisterValue(uint16_t* data, size_t count, DataType type) {
    RegisterValue value;
    value.type = type;
    
    switch (type) {
        case DataType::BOOL:
            value.setBool(data[0] != 0);
            break;
        
        case DataType::UINT8:
            value.setUint8(static_cast<uint8_t>(data[0]));
            break;
        
        case DataType::INT8:
            value.setInt8(static_cast<int8_t>(data[0]));
            break;
        
        case DataType::UINT16:
            value.setUint16(data[0]);
            break;
        
        case DataType::INT16:
            value.setInt16(static_cast<int16_t>(data[0]));
            break;
        
        case DataType::UINT32:
            if (count >= 2) {
                uint32_t val = (static_cast<uint32_t>(data[0]) << 16) | data[1];
                value.setUint32(val);
            } else {
                value.setUint32(data[0]);
            }
            break;
        
        case DataType::INT32:
            if (count >= 2) {
                int32_t val = (static_cast<int32_t>(data[0]) << 16) | data[1];
                value.setInt32(val);
            } else {
                value.setInt32(static_cast<int32_t>(data[0]));
            }
            break;
        
        case DataType::FLOAT32:
            if (count >= 2) {
                uint32_t bits = (static_cast<uint32_t>(data[0]) << 16) | data[1];
                float val;
                std::memcpy(&val, &bits, sizeof(val));
                value.setFloat32(val);
            } else {
                value.setFloat32(0.0f);
            }
            break;
        
        case DataType::UINT64:
            if (count >= 4) {
                uint64_t val = (static_cast<uint64_t>(data[0]) << 48) |
                              (static_cast<uint64_t>(data[1]) << 32) |
                              (static_cast<uint64_t>(data[2]) << 16) |
                              data[3];
                value.setUint64(val);
            } else if (count >= 2) {
                uint64_t val = (static_cast<uint64_t>(data[0]) << 16) | data[1];
                value.setUint64(val);
            } else {
                value.setUint64(data[0]);
            }
            break;
        
        case DataType::INT64:
            if (count >= 4) {
                int64_t val = (static_cast<int64_t>(data[0]) << 48) |
                             (static_cast<int64_t>(data[1]) << 32) |
                             (static_cast<int64_t>(data[2]) << 16) |
                             data[3];
                value.setInt64(val);
            } else if (count >= 2) {
                int64_t val = (static_cast<int64_t>(data[0]) << 16) | data[1];
                value.setInt64(val);
            } else {
                value.setInt64(static_cast<int64_t>(data[0]));
            }
            break;
        
        case DataType::FLOAT64:
            if (count >= 4) {
                uint64_t bits = (static_cast<uint64_t>(data[0]) << 48) |
                               (static_cast<uint64_t>(data[1]) << 32) |
                               (static_cast<uint64_t>(data[2]) << 16) |
                               data[3];
                double val;
                std::memcpy(&val, &bits, sizeof(val));
                value.setFloat64(val);
            } else {
                value.setFloat64(0.0);
            }
            break;
        
        case DataType::STRING:
        case DataType::BINARY:
        default:
            // Convert to binary data
            value.type = DataType::BINARY;
            value.data.resize(count * 2);
            for (size_t i = 0; i < count; ++i) {
                value.data[i * 2] = static_cast<uint8_t>(data[i] >> 8);
                value.data[i * 2 + 1] = static_cast<uint8_t>(data[i] & 0xFF);
            }
            break;
    }
    
    return value;
}

std::vector<uint16_t> ModbusRtuAdapter::convertFromRegisterValue(const RegisterValue& value, size_t& count) {
    std::vector<uint16_t> result;
    
    switch (value.type) {
        case DataType::BOOL:
            result.push_back(value.getBool() ? 1 : 0);
            count = 1;
            break;
        
        case DataType::UINT8:
        case DataType::INT8:
            result.push_back(value.getUint8());
            count = 1;
            break;
        
        case DataType::UINT16:
        case DataType::INT16:
            result.push_back(value.getUint16());
            count = 1;
            break;
        
        case DataType::UINT32:
        case DataType::INT32:
        case DataType::FLOAT32: {
            uint32_t val;
            if (value.type == DataType::FLOAT32) {
                float f = value.getFloat32();
                std::memcpy(&val, &f, sizeof(val));
            } else {
                val = value.getUint32();
            }
            
            result.push_back(static_cast<uint16_t>(val >> 16));
            result.push_back(static_cast<uint16_t>(val & 0xFFFF));
            count = 2;
            break;
        }
        
        case DataType::UINT64:
        case DataType::INT64:
        case DataType::FLOAT64: {
            uint64_t val;
            if (value.type == DataType::FLOAT64) {
                double d = value.getFloat64();
                std::memcpy(&val, &d, sizeof(val));
            } else {
                val = value.getUint64();
            }
            
            result.push_back(static_cast<uint16_t>(val >> 48));
            result.push_back(static_cast<uint16_t>((val >> 32) & 0xFFFF));
            result.push_back(static_cast<uint16_t>((val >> 16) & 0xFFFF));
            result.push_back(static_cast<uint16_t>(val & 0xFFFF));
            count = 4;
            break;
        }
        
        case DataType::STRING:
        case DataType::BINARY:
        default:
            // Convert from binary data
            count = (value.data.size() + 1) / 2;
            result.resize(count);
            
            for (size_t i = 0; i < count; ++i) {
                uint16_t high = (i * 2 < value.data.size()) ? value.data[i * 2] : 0;
                uint16_t low = (i * 2 + 1 < value.data.size()) ? value.data[i * 2 + 1] : 0;
                result[i] = (high << 8) | low;
            }
            break;
    }
    
    return result;
}

std::vector<ModbusRtuAdapter::RegisterGroup> ModbusRtuAdapter::groupRegisters(const std::vector<RegisterAddress>& addresses) {
    std::vector<RegisterGroup> groups;
    
    // Group by register type
    std::map<RegisterType, std::vector<RegisterAddress>> type_groups;
    for (const auto& address : addresses) {
        type_groups[address.type].push_back(address);
    }
    
    // Process each type group
    for (const auto& type_group : type_groups) {
        const auto& type = type_group.first;
        const auto& addrs = type_group.second;
        
        // Sort addresses by address
        std::vector<RegisterAddress> sorted_addrs = addrs;
        std::sort(sorted_addrs.begin(), sorted_addrs.end(), 
                 [](const RegisterAddress& a, const RegisterAddress& b) {
                     return a.address < b.address;
                 });
        
        // Group contiguous addresses
        uint32_t current_start = sorted_addrs[0].address;
        uint32_t current_end = current_start + sorted_addrs[0].count;
        std::vector<RegisterAddress> current_group = {sorted_addrs[0]};
        
        for (size_t i = 1; i < sorted_addrs.size(); ++i) {
            const auto& addr = sorted_addrs[i];
            
            // Check if this address is contiguous with the current group
            if (addr.address <= current_end) {
                // Extend the current group
                current_end = std::max(current_end, addr.address + addr.count);
                current_group.push_back(addr);
            } else {
                // Start a new group
                RegisterGroup group;
                group.type = type;
                group.start_address = current_start;
                group.count = current_end - current_start;
                group.addresses = current_group;
                groups.push_back(group);
                
                current_start = addr.address;
                current_end = current_start + addr.count;
                current_group = {addr};
            }
        }
        
        // Add the last group
        RegisterGroup group;
        group.type = type;
        group.start_address = current_start;
        group.count = current_end - current_start;
        group.addresses = current_group;
        groups.push_back(group);
    }
    
    return groups;
}

ReadResult ModbusRtuAdapter::handleModbusError(const std::string& operation, int error_code) {
    std::string error_message = "Failed to " + operation + ": " + modbus_strerror(error_code);
    LOG_ERROR(error_message);
    return createErrorReadResult(error_message, error_code);
}

WriteResult ModbusRtuAdapter::handleModbusWriteError(const std::string& operation, int error_code) {
    std::string error_message = "Failed to " + operation + ": " + modbus_strerror(error_code);
    LOG_ERROR(error_message);
    return createErrorWriteResult(error_message, error_code);
}

void ModbusRtuAdapter::startDeviceStatusMonitoring() {
    status_monitor_running_ = true;
    status_monitor_thread_ = std::thread(&ModbusRtuAdapter::statusMonitorThreadFunc, this);
}

void ModbusRtuAdapter::stopDeviceStatusMonitoring() {
    status_monitor_running_ = false;
    if (status_monitor_thread_.joinable()) {
        status_monitor_thread_.join();
    }
}

void ModbusRtuAdapter::statusMonitorThreadFunc() {
    LOG_INFO("Modbus RTU status monitor thread started");
    
    while (status_monitor_running_) {
        // Sleep for the monitoring interval
        std::this_thread::sleep_for(std::chrono::milliseconds(STATUS_MONITOR_INTERVAL_MS));
        
        // Get all devices
        auto devices = getAllDevices();
        
        // Check each device
        for (const auto& device : devices) {
            // Skip already offline devices
            if (!device.online) {
                continue;
            }
            
            // Get Modbus connection
            auto connection = getConnection(device.id);
            if (!connection || !connection->connected) {
                // Device is offline
                updateDeviceStatus(device.id, false);
                continue;
            }
            
            // Get unit ID
            auto modbus_address = std::dynamic_pointer_cast<ModbusRtuAddress>(device.address);
            if (!modbus_address) {
                continue;
            }
            
            int unit_id = modbus_address->unit_id;
            
            // Lock the connection for thread safety
            std::lock_guard<std::mutex> lock(connection->mutex);
            
            // Set the slave ID
            modbus_set_slave(connection->ctx, unit_id);
            
            // Try to read a holding register (common register type)
            uint16_t reg_value;
            int rc = modbus_read_registers(connection->ctx, 0, 1, &reg_value);
            
            // Update device status
            updateDeviceStatus(device.id, rc == 1);
        }
    }
    
    LOG_INFO("Modbus RTU status monitor thread stopped");
}

} // namespace device
} // namespace ocpp_gateway

#else // MODBUS_SUPPORT_ENABLED

// Stub implementation when modbus is not available
namespace ocpp_gateway {
namespace device {

ModbusRtuAdapter::ModbusRtuAdapter()
    : BaseDeviceAdapter(DeviceProtocol::MODBUS_RTU) {
}

ModbusRtuAdapter::~ModbusRtuAdapter() {
}

bool ModbusRtuAdapter::initialize() {
    LOG_WARN("Modbus RTU adapter not available - libmodbus not found");
    return false;
}

bool ModbusRtuAdapter::start() {
    LOG_WARN("Modbus RTU adapter not available - libmodbus not found");
    return false;
}

void ModbusRtuAdapter::stop() {
}

bool ModbusRtuAdapter::startDiscovery(DeviceDiscoveryCallback callback, 
                                    std::chrono::milliseconds timeout_ms) {
    LOG_WARN("Modbus RTU discovery not available - libmodbus not found");
    return false;
}

void ModbusRtuAdapter::stopDiscovery() {
}

bool ModbusRtuAdapter::isDiscoveryInProgress() const {
    return false;
}

ReadResult ModbusRtuAdapter::readRegister(const std::string& device_id, const RegisterAddress& address) {
    return createErrorReadResult("Modbus RTU not available - libmodbus not found", -1);
}

WriteResult ModbusRtuAdapter::writeRegister(const std::string& device_id, const RegisterAddress& address, 
                                           const RegisterValue& value) {
    return createErrorWriteResult("Modbus RTU not available - libmodbus not found", -1);
}

std::map<RegisterAddress, ReadResult> ModbusRtuAdapter::readMultipleRegisters(
    const std::string& device_id, const std::vector<RegisterAddress>& addresses) {
    return {};
}

std::map<RegisterAddress, WriteResult> ModbusRtuAdapter::writeMultipleRegisters(
    const std::string& device_id, const std::map<RegisterAddress, RegisterValue>& values) {
    return {};
}

bool ModbusRtuAdapter::validateDeviceAddress(const DeviceInfo& device_info) const {
    return false;
}

bool ModbusRtuAdapter::validateRegisterAddress(const RegisterAddress& address) const {
    return false;
}

bool ModbusRtuAdapter::validateRegisterValue(const RegisterAddress& address, const RegisterValue& value) const {
    return false;
}

} // namespace device
} // namespace ocpp_gateway

#endif // MODBUS_SUPPORT_ENABLED