#include "ocpp_gateway/common/device_config.h"
#include <yaml-cpp/yaml.h>
#include <json/json.h>
#include <fstream>
#include <sstream>

namespace ocpp_gateway {
namespace config {

DeviceConfig::DeviceConfig()
    : id_(""),
      template_id_(""),
      protocol_(ProtocolType::UNKNOWN),
      connection_(ModbusTcpConnectionConfig{}),
      ocpp_id_("") {
    // Default values are set in the initializer list
}

DeviceConfig::DeviceConfig(
    const std::string& id,
    const std::string& template_id,
    ProtocolType protocol,
    const ConnectionConfig& connection,
    const std::string& ocpp_id
)
    : id_(id),
      template_id_(template_id),
      protocol_(protocol),
      connection_(connection),
      ocpp_id_(ocpp_id) {
}

bool DeviceConfig::loadFromYaml(const std::string& yaml_file) {
    try {
        YAML::Node config = YAML::LoadFile(yaml_file);
        
        if (!config["device"]) {
            return false;
        }

        YAML::Node device = config["device"];
        
        // Load ID
        if (device["id"]) {
            id_ = device["id"].as<std::string>();
        }
        
        // Load template ID
        if (device["template"]) {
            template_id_ = device["template"].as<std::string>();
        }
        
        // Load protocol
        if (device["protocol"]) {
            protocol_ = protocolFromString(device["protocol"].as<std::string>());
        }
        
        // Load OCPP ID
        if (device["ocpp_id"]) {
            ocpp_id_ = device["ocpp_id"].as<std::string>();
        }
        
        // Load connection based on protocol
        if (device["connection"]) {
            YAML::Node connection = device["connection"];
            
            switch (protocol_) {
                case ProtocolType::MODBUS_TCP: {
                    ModbusTcpConnectionConfig conn;
                    
                    if (connection["ip"]) {
                        conn.ip = connection["ip"].as<std::string>();
                    }
                    
                    if (connection["port"]) {
                        conn.port = connection["port"].as<int>();
                    }
                    
                    if (connection["unit_id"]) {
                        conn.unit_id = connection["unit_id"].as<int>();
                    }
                    
                    connection_ = conn;
                    break;
                }
                case ProtocolType::MODBUS_RTU: {
                    ModbusRtuConnectionConfig conn;
                    
                    if (connection["port"]) {
                        conn.port = connection["port"].as<std::string>();
                    }
                    
                    if (connection["baud_rate"]) {
                        conn.baud_rate = connection["baud_rate"].as<int>();
                    }
                    
                    if (connection["data_bits"]) {
                        conn.data_bits = connection["data_bits"].as<int>();
                    }
                    
                    if (connection["stop_bits"]) {
                        conn.stop_bits = connection["stop_bits"].as<int>();
                    }
                    
                    if (connection["parity"]) {
                        conn.parity = connection["parity"].as<std::string>();
                    }
                    
                    if (connection["unit_id"]) {
                        conn.unit_id = connection["unit_id"].as<int>();
                    }
                    
                    connection_ = conn;
                    break;
                }
                case ProtocolType::ECHONET_LITE: {
                    EchonetLiteConnectionConfig conn;
                    
                    if (connection["ip"]) {
                        conn.ip = connection["ip"].as<std::string>();
                    }
                    
                    connection_ = conn;
                    break;
                }
                default:
                    return false;
            }
        }
        
        return validate();
    } catch (const YAML::Exception& e) {
        // Log error
        return false;
    } catch (const ConfigValidationError& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool DeviceConfig::loadFromYamlString(const std::string& yaml_content) {
    try {
        YAML::Node config = YAML::Load(yaml_content);
        
        if (!config["device"]) {
            return false;
        }

        YAML::Node device = config["device"];
        
        // Load ID
        if (device["id"]) {
            id_ = device["id"].as<std::string>();
        }
        
        // Load template ID
        if (device["template"]) {
            template_id_ = device["template"].as<std::string>();
        }
        
        // Load protocol
        if (device["protocol"]) {
            protocol_ = protocolFromString(device["protocol"].as<std::string>());
        }
        
        // Load OCPP ID
        if (device["ocpp_id"]) {
            ocpp_id_ = device["ocpp_id"].as<std::string>();
        }
        
        // Load connection based on protocol
        if (device["connection"]) {
            YAML::Node connection = device["connection"];
            
            switch (protocol_) {
                case ProtocolType::MODBUS_TCP: {
                    ModbusTcpConnectionConfig conn;
                    
                    if (connection["ip"]) {
                        conn.ip = connection["ip"].as<std::string>();
                    }
                    
                    if (connection["port"]) {
                        conn.port = connection["port"].as<int>();
                    }
                    
                    if (connection["unit_id"]) {
                        conn.unit_id = connection["unit_id"].as<int>();
                    }
                    
                    connection_ = conn;
                    break;
                }
                case ProtocolType::MODBUS_RTU: {
                    ModbusRtuConnectionConfig conn;
                    
                    if (connection["port"]) {
                        conn.port = connection["port"].as<std::string>();
                    }
                    
                    if (connection["baud_rate"]) {
                        conn.baud_rate = connection["baud_rate"].as<int>();
                    }
                    
                    if (connection["data_bits"]) {
                        conn.data_bits = connection["data_bits"].as<int>();
                    }
                    
                    if (connection["stop_bits"]) {
                        conn.stop_bits = connection["stop_bits"].as<int>();
                    }
                    
                    if (connection["parity"]) {
                        conn.parity = connection["parity"].as<std::string>();
                    }
                    
                    if (connection["unit_id"]) {
                        conn.unit_id = connection["unit_id"].as<int>();
                    }
                    
                    connection_ = conn;
                    break;
                }
                case ProtocolType::ECHONET_LITE: {
                    EchonetLiteConnectionConfig conn;
                    
                    if (connection["ip"]) {
                        conn.ip = connection["ip"].as<std::string>();
                    }
                    
                    connection_ = conn;
                    break;
                }
                default:
                    return false;
            }
        }
        
        return validate();
    } catch (const YAML::Exception& e) {
        // Log error
        return false;
    } catch (const ConfigValidationError& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool DeviceConfig::loadFromJson(const std::string& json_file) {
    try {
        std::ifstream file(json_file);
        if (!file.is_open()) {
            return false;
        }
        
        Json::Value root;
        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;
        if (!Json::parseFromStream(builder, file, &root, &errs)) {
            return false;
        }
        
        if (!root.isMember("device")) {
            return false;
        }
        
        const Json::Value& device = root["device"];
        
        // Load ID
        if (device.isMember("id")) {
            id_ = device["id"].asString();
        }
        
        // Load template ID
        if (device.isMember("template")) {
            template_id_ = device["template"].asString();
        }
        
        // Load protocol
        if (device.isMember("protocol")) {
            protocol_ = protocolFromString(device["protocol"].asString());
        }
        
        // Load OCPP ID
        if (device.isMember("ocpp_id")) {
            ocpp_id_ = device["ocpp_id"].asString();
        }
        
        // Load connection based on protocol
        if (device.isMember("connection")) {
            const Json::Value& connection = device["connection"];
            
            switch (protocol_) {
                case ProtocolType::MODBUS_TCP: {
                    ModbusTcpConnectionConfig conn;
                    
                    if (connection.isMember("ip")) {
                        conn.ip = connection["ip"].asString();
                    }
                    
                    if (connection.isMember("port")) {
                        conn.port = connection["port"].asInt();
                    }
                    
                    if (connection.isMember("unit_id")) {
                        conn.unit_id = connection["unit_id"].asInt();
                    }
                    
                    connection_ = conn;
                    break;
                }
                case ProtocolType::MODBUS_RTU: {
                    ModbusRtuConnectionConfig conn;
                    
                    if (connection.isMember("port")) {
                        conn.port = connection["port"].asString();
                    }
                    
                    if (connection.isMember("baud_rate")) {
                        conn.baud_rate = connection["baud_rate"].asInt();
                    }
                    
                    if (connection.isMember("data_bits")) {
                        conn.data_bits = connection["data_bits"].asInt();
                    }
                    
                    if (connection.isMember("stop_bits")) {
                        conn.stop_bits = connection["stop_bits"].asInt();
                    }
                    
                    if (connection.isMember("parity")) {
                        conn.parity = connection["parity"].asString();
                    }
                    
                    if (connection.isMember("unit_id")) {
                        conn.unit_id = connection["unit_id"].asInt();
                    }
                    
                    connection_ = conn;
                    break;
                }
                case ProtocolType::ECHONET_LITE: {
                    EchonetLiteConnectionConfig conn;
                    
                    if (connection.isMember("ip")) {
                        conn.ip = connection["ip"].asString();
                    }
                    
                    connection_ = conn;
                    break;
                }
                default:
                    return false;
            }
        }
        
        return validate();
    } catch (const Json::Exception& e) {
        // Log error
        return false;
    } catch (const ConfigValidationError& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool DeviceConfig::loadFromJsonString(const std::string& json_content) {
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        if (!reader->parse(json_content.c_str(), json_content.c_str() + json_content.length(), &root, &errs)) {
            return false;
        }
        
        if (!root.isMember("device")) {
            return false;
        }
        
        const Json::Value& device = root["device"];
        
        // Load ID
        if (device.isMember("id")) {
            id_ = device["id"].asString();
        }
        
        // Load template ID
        if (device.isMember("template")) {
            template_id_ = device["template"].asString();
        }
        
        // Load protocol
        if (device.isMember("protocol")) {
            protocol_ = protocolFromString(device["protocol"].asString());
        }
        
        // Load OCPP ID
        if (device.isMember("ocpp_id")) {
            ocpp_id_ = device["ocpp_id"].asString();
        }
        
        // Load connection based on protocol
        if (device.isMember("connection")) {
            const Json::Value& connection = device["connection"];
            
            switch (protocol_) {
                case ProtocolType::MODBUS_TCP: {
                    ModbusTcpConnectionConfig conn;
                    
                    if (connection.isMember("ip")) {
                        conn.ip = connection["ip"].asString();
                    }
                    
                    if (connection.isMember("port")) {
                        conn.port = connection["port"].asInt();
                    }
                    
                    if (connection.isMember("unit_id")) {
                        conn.unit_id = connection["unit_id"].asInt();
                    }
                    
                    connection_ = conn;
                    break;
                }
                case ProtocolType::MODBUS_RTU: {
                    ModbusRtuConnectionConfig conn;
                    
                    if (connection.isMember("port")) {
                        conn.port = connection["port"].asString();
                    }
                    
                    if (connection.isMember("baud_rate")) {
                        conn.baud_rate = connection["baud_rate"].asInt();
                    }
                    
                    if (connection.isMember("data_bits")) {
                        conn.data_bits = connection["data_bits"].asInt();
                    }
                    
                    if (connection.isMember("stop_bits")) {
                        conn.stop_bits = connection["stop_bits"].asInt();
                    }
                    
                    if (connection.isMember("parity")) {
                        conn.parity = connection["parity"].asString();
                    }
                    
                    if (connection.isMember("unit_id")) {
                        conn.unit_id = connection["unit_id"].asInt();
                    }
                    
                    connection_ = conn;
                    break;
                }
                case ProtocolType::ECHONET_LITE: {
                    EchonetLiteConnectionConfig conn;
                    
                    if (connection.isMember("ip")) {
                        conn.ip = connection["ip"].asString();
                    }
                    
                    connection_ = conn;
                    break;
                }
                default:
                    return false;
            }
        }
        
        return validate();
    } catch (const Json::Exception& e) {
        // Log error
        return false;
    } catch (const ConfigValidationError& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool DeviceConfig::saveToYaml(const std::string& yaml_file) const {
    try {
        YAML::Node device;
        
        // Save ID
        device["id"] = id_;
        
        // Save template ID
        device["template"] = template_id_;
        
        // Save protocol
        device["protocol"] = protocolToString(protocol_);
        
        // Save OCPP ID
        device["ocpp_id"] = ocpp_id_;
        
        // Save connection based on protocol
        YAML::Node connection;
        
        switch (protocol_) {
            case ProtocolType::MODBUS_TCP: {
                const auto& conn = std::get<ModbusTcpConnectionConfig>(connection_);
                connection["ip"] = conn.ip;
                connection["port"] = conn.port;
                connection["unit_id"] = conn.unit_id;
                break;
            }
            case ProtocolType::MODBUS_RTU: {
                const auto& conn = std::get<ModbusRtuConnectionConfig>(connection_);
                connection["port"] = conn.port;
                connection["baud_rate"] = conn.baud_rate;
                connection["data_bits"] = conn.data_bits;
                connection["stop_bits"] = conn.stop_bits;
                connection["parity"] = conn.parity;
                connection["unit_id"] = conn.unit_id;
                break;
            }
            case ProtocolType::ECHONET_LITE: {
                const auto& conn = std::get<EchonetLiteConnectionConfig>(connection_);
                connection["ip"] = conn.ip;
                break;
            }
            default:
                return false;
        }
        
        device["connection"] = connection;
        
        YAML::Node root;
        root["device"] = device;
        
        std::ofstream file(yaml_file);
        if (!file.is_open()) {
            return false;
        }
        
        file << root;
        return true;
    } catch (const YAML::Exception& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool DeviceConfig::saveToJson(const std::string& json_file) const {
    try {
        Json::Value device;
        
        // Save ID
        device["id"] = id_;
        
        // Save template ID
        device["template"] = template_id_;
        
        // Save protocol
        device["protocol"] = protocolToString(protocol_);
        
        // Save OCPP ID
        device["ocpp_id"] = ocpp_id_;
        
        // Save connection based on protocol
        Json::Value connection;
        
        switch (protocol_) {
            case ProtocolType::MODBUS_TCP: {
                const auto& conn = std::get<ModbusTcpConnectionConfig>(connection_);
                connection["ip"] = conn.ip;
                connection["port"] = conn.port;
                connection["unit_id"] = conn.unit_id;
                break;
            }
            case ProtocolType::MODBUS_RTU: {
                const auto& conn = std::get<ModbusRtuConnectionConfig>(connection_);
                connection["port"] = conn.port;
                connection["baud_rate"] = conn.baud_rate;
                connection["data_bits"] = conn.data_bits;
                connection["stop_bits"] = conn.stop_bits;
                connection["parity"] = conn.parity;
                connection["unit_id"] = conn.unit_id;
                break;
            }
            case ProtocolType::ECHONET_LITE: {
                const auto& conn = std::get<EchonetLiteConnectionConfig>(connection_);
                connection["ip"] = conn.ip;
                break;
            }
            default:
                return false;
        }
        
        device["connection"] = connection;
        
        Json::Value root;
        root["device"] = device;
        
        std::ofstream file(json_file);
        if (!file.is_open()) {
            return false;
        }
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        writer->write(root, &file);
        
        return true;
    } catch (const Json::Exception& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool DeviceConfig::validate() const {
    try {
        if (id_.empty()) {
            throw ConfigValidationError("Device ID cannot be empty");
        }
        
        if (template_id_.empty()) {
            throw ConfigValidationError("Device template ID cannot be empty");
        }
        
        if (protocol_ == ProtocolType::UNKNOWN) {
            throw ConfigValidationError("Device protocol is unknown or invalid");
        }
        
        if (ocpp_id_.empty()) {
            throw ConfigValidationError("Device OCPP ID cannot be empty");
        }
        
        // Validate connection based on protocol
        switch (protocol_) {
            case ProtocolType::MODBUS_TCP: {
                const auto& conn = std::get<ModbusTcpConnectionConfig>(connection_);
                conn.validate();
                break;
            }
            case ProtocolType::MODBUS_RTU: {
                const auto& conn = std::get<ModbusRtuConnectionConfig>(connection_);
                conn.validate();
                break;
            }
            case ProtocolType::ECHONET_LITE: {
                const auto& conn = std::get<EchonetLiteConnectionConfig>(connection_);
                conn.validate();
                break;
            }
            default:
                throw ConfigValidationError("Device protocol is unknown or invalid");
        }
        
        return true;
    } catch (const ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw ConfigValidationError(std::string("Unexpected error during validation: ") + e.what());
    }
}

// DeviceConfigCollection implementation

DeviceConfigCollection::DeviceConfigCollection() {
    // Default constructor
}

DeviceConfigCollection::DeviceConfigCollection(const std::string& yaml_file) {
    loadFromYaml(yaml_file);
}

bool DeviceConfigCollection::loadFromYaml(const std::string& yaml_file) {
    try {
        YAML::Node config = YAML::LoadFile(yaml_file);
        
        if (!config["devices"]) {
            return false;
        }

        YAML::Node devices = config["devices"];
        
        if (!devices.IsSequence()) {
            return false;
        }
        
        devices_.clear();
        
        for (const auto& device_node : devices) {
            DeviceConfig device;
            
            // Load ID
            if (device_node["id"]) {
                device.setId(device_node["id"].as<std::string>());
            }
            
            // Load template ID
            if (device_node["template"]) {
                device.setTemplateId(device_node["template"].as<std::string>());
            }
            
            // Load protocol
            if (device_node["protocol"]) {
                device.setProtocol(protocolFromString(device_node["protocol"].as<std::string>()));
            }
            
            // Load OCPP ID
            if (device_node["ocpp_id"]) {
                device.setOcppId(device_node["ocpp_id"].as<std::string>());
            }
            
            // Load connection based on protocol
            if (device_node["connection"]) {
                YAML::Node connection = device_node["connection"];
                
                switch (device.getProtocol()) {
                    case ProtocolType::MODBUS_TCP: {
                        ModbusTcpConnectionConfig conn;
                        
                        if (connection["ip"]) {
                            conn.ip = connection["ip"].as<std::string>();
                        }
                        
                        if (connection["port"]) {
                            conn.port = connection["port"].as<int>();
                        }
                        
                        if (connection["unit_id"]) {
                            conn.unit_id = connection["unit_id"].as<int>();
                        }
                        
                        device.setConnection(conn);
                        break;
                    }
                    case ProtocolType::MODBUS_RTU: {
                        ModbusRtuConnectionConfig conn;
                        
                        if (connection["port"]) {
                            conn.port = connection["port"].as<std::string>();
                        }
                        
                        if (connection["baud_rate"]) {
                            conn.baud_rate = connection["baud_rate"].as<int>();
                        }
                        
                        if (connection["data_bits"]) {
                            conn.data_bits = connection["data_bits"].as<int>();
                        }
                        
                        if (connection["stop_bits"]) {
                            conn.stop_bits = connection["stop_bits"].as<int>();
                        }
                        
                        if (connection["parity"]) {
                            conn.parity = connection["parity"].as<std::string>();
                        }
                        
                        if (connection["unit_id"]) {
                            conn.unit_id = connection["unit_id"].as<int>();
                        }
                        
                        device.setConnection(conn);
                        break;
                    }
                    case ProtocolType::ECHONET_LITE: {
                        EchonetLiteConnectionConfig conn;
                        
                        if (connection["ip"]) {
                            conn.ip = connection["ip"].as<std::string>();
                        }
                        
                        device.setConnection(conn);
                        break;
                    }
                    default:
                        continue;
                }
            }
            
            try {
                if (device.validate()) {
                    devices_.push_back(device);
                }
            } catch (const ConfigValidationError& e) {
                // Log error and skip this device
                continue;
            }
        }
        
        return true;
    } catch (const YAML::Exception& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool DeviceConfigCollection::loadFromYamlString(const std::string& yaml_content) {
    try {
        YAML::Node config = YAML::Load(yaml_content);
        
        if (!config["devices"]) {
            return false;
        }

        YAML::Node devices = config["devices"];
        
        if (!devices.IsSequence()) {
            return false;
        }
        
        devices_.clear();
        
        for (const auto& device_node : devices) {
            DeviceConfig device;
            
            // Load ID
            if (device_node["id"]) {
                device.setId(device_node["id"].as<std::string>());
            }
            
            // Load template ID
            if (device_node["template"]) {
                device.setTemplateId(device_node["template"].as<std::string>());
            }
            
            // Load protocol
            if (device_node["protocol"]) {
                device.setProtocol(protocolFromString(device_node["protocol"].as<std::string>()));
            }
            
            // Load OCPP ID
            if (device_node["ocpp_id"]) {
                device.setOcppId(device_node["ocpp_id"].as<std::string>());
            }
            
            // Load connection based on protocol
            if (device_node["connection"]) {
                YAML::Node connection = device_node["connection"];
                
                switch (device.getProtocol()) {
                    case ProtocolType::MODBUS_TCP: {
                        ModbusTcpConnectionConfig conn;
                        
                        if (connection["ip"]) {
                            conn.ip = connection["ip"].as<std::string>();
                        }
                        
                        if (connection["port"]) {
                            conn.port = connection["port"].as<int>();
                        }
                        
                        if (connection["unit_id"]) {
                            conn.unit_id = connection["unit_id"].as<int>();
                        }
                        
                        device.setConnection(conn);
                        break;
                    }
                    case ProtocolType::MODBUS_RTU: {
                        ModbusRtuConnectionConfig conn;
                        
                        if (connection["port"]) {
                            conn.port = connection["port"].as<std::string>();
                        }
                        
                        if (connection["baud_rate"]) {
                            conn.baud_rate = connection["baud_rate"].as<int>();
                        }
                        
                        if (connection["data_bits"]) {
                            conn.data_bits = connection["data_bits"].as<int>();
                        }
                        
                        if (connection["stop_bits"]) {
                            conn.stop_bits = connection["stop_bits"].as<int>();
                        }
                        
                        if (connection["parity"]) {
                            conn.parity = connection["parity"].as<std::string>();
                        }
                        
                        if (connection["unit_id"]) {
                            conn.unit_id = connection["unit_id"].as<int>();
                        }
                        
                        device.setConnection(conn);
                        break;
                    }
                    case ProtocolType::ECHONET_LITE: {
                        EchonetLiteConnectionConfig conn;
                        
                        if (connection["ip"]) {
                            conn.ip = connection["ip"].as<std::string>();
                        }
                        
                        device.setConnection(conn);
                        break;
                    }
                    default:
                        continue;
                }
            }
            
            try {
                if (device.validate()) {
                    devices_.push_back(device);
                }
            } catch (const ConfigValidationError& e) {
                // Log error and skip this device
                continue;
            }
        }
        
        return true;
    } catch (const YAML::Exception& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool DeviceConfigCollection::loadFromJson(const std::string& json_file) {
    try {
        std::ifstream file(json_file);
        if (!file.is_open()) {
            return false;
        }
        
        Json::Value root;
        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;
        if (!Json::parseFromStream(builder, file, &root, &errs)) {
            return false;
        }
        
        if (!root.isMember("devices") || !root["devices"].isArray()) {
            return false;
        }
        
        const Json::Value& devices = root["devices"];
        
        devices_.clear();
        
        for (const auto& device_node : devices) {
            DeviceConfig device;
            
            // Load ID
            if (device_node.isMember("id")) {
                device.setId(device_node["id"].asString());
            }
            
            // Load template ID
            if (device_node.isMember("template")) {
                device.setTemplateId(device_node["template"].asString());
            }
            
            // Load protocol
            if (device_node.isMember("protocol")) {
                device.setProtocol(protocolFromString(device_node["protocol"].asString()));
            }
            
            // Load OCPP ID
            if (device_node.isMember("ocpp_id")) {
                device.setOcppId(device_node["ocpp_id"].asString());
            }
            
            // Load connection based on protocol
            if (device_node.isMember("connection")) {
                const Json::Value& connection = device_node["connection"];
                
                switch (device.getProtocol()) {
                    case ProtocolType::MODBUS_TCP: {
                        ModbusTcpConnectionConfig conn;
                        
                        if (connection.isMember("ip")) {
                            conn.ip = connection["ip"].asString();
                        }
                        
                        if (connection.isMember("port")) {
                            conn.port = connection["port"].asInt();
                        }
                        
                        if (connection.isMember("unit_id")) {
                            conn.unit_id = connection["unit_id"].asInt();
                        }
                        
                        device.setConnection(conn);
                        break;
                    }
                    case ProtocolType::MODBUS_RTU: {
                        ModbusRtuConnectionConfig conn;
                        
                        if (connection.isMember("port")) {
                            conn.port = connection["port"].asString();
                        }
                        
                        if (connection.isMember("baud_rate")) {
                            conn.baud_rate = connection["baud_rate"].asInt();
                        }
                        
                        if (connection.isMember("data_bits")) {
                            conn.data_bits = connection["data_bits"].asInt();
                        }
                        
                        if (connection.isMember("stop_bits")) {
                            conn.stop_bits = connection["stop_bits"].asInt();
                        }
                        
                        if (connection.isMember("parity")) {
                            conn.parity = connection["parity"].asString();
                        }
                        
                        if (connection.isMember("unit_id")) {
                            conn.unit_id = connection["unit_id"].asInt();
                        }
                        
                        device.setConnection(conn);
                        break;
                    }
                    case ProtocolType::ECHONET_LITE: {
                        EchonetLiteConnectionConfig conn;
                        
                        if (connection.isMember("ip")) {
                            conn.ip = connection["ip"].asString();
                        }
                        
                        device.setConnection(conn);
                        break;
                    }
                    default:
                        continue;
                }
            }
            
            try {
                if (device.validate()) {
                    devices_.push_back(device);
                }
            } catch (const ConfigValidationError& e) {
                // Log error and skip this device
                continue;
            }
        }
        
        return true;
    } catch (const Json::Exception& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool DeviceConfigCollection::loadFromJsonString(const std::string& json_content) {
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        if (!reader->parse(json_content.c_str(), json_content.c_str() + json_content.length(), &root, &errs)) {
            return false;
        }
        
        if (!root.isMember("devices") || !root["devices"].isArray()) {
            return false;
        }
        
        const Json::Value& devices = root["devices"];
        
        devices_.clear();
        
        for (const auto& device_node : devices) {
            DeviceConfig device;
            
            // Load ID
            if (device_node.isMember("id")) {
                device.setId(device_node["id"].asString());
            }
            
            // Load template ID
            if (device_node.isMember("template")) {
                device.setTemplateId(device_node["template"].asString());
            }
            
            // Load protocol
            if (device_node.isMember("protocol")) {
                device.setProtocol(protocolFromString(device_node["protocol"].asString()));
            }
            
            // Load OCPP ID
            if (device_node.isMember("ocpp_id")) {
                device.setOcppId(device_node["ocpp_id"].asString());
            }
            
            // Load connection based on protocol
            if (device_node.isMember("connection")) {
                const Json::Value& connection = device_node["connection"];
                
                switch (device.getProtocol()) {
                    case ProtocolType::MODBUS_TCP: {
                        ModbusTcpConnectionConfig conn;
                        
                        if (connection.isMember("ip")) {
                            conn.ip = connection["ip"].asString();
                        }
                        
                        if (connection.isMember("port")) {
                            conn.port = connection["port"].asInt();
                        }
                        
                        if (connection.isMember("unit_id")) {
                            conn.unit_id = connection["unit_id"].asInt();
                        }
                        
                        device.setConnection(conn);
                        break;
                    }
                    case ProtocolType::MODBUS_RTU: {
                        ModbusRtuConnectionConfig conn;
                        
                        if (connection.isMember("port")) {
                            conn.port = connection["port"].asString();
                        }
                        
                        if (connection.isMember("baud_rate")) {
                            conn.baud_rate = connection["baud_rate"].asInt();
                        }
                        
                        if (connection.isMember("data_bits")) {
                            conn.data_bits = connection["data_bits"].asInt();
                        }
                        
                        if (connection.isMember("stop_bits")) {
                            conn.stop_bits = connection["stop_bits"].asInt();
                        }
                        
                        if (connection.isMember("parity")) {
                            conn.parity = connection["parity"].asString();
                        }
                        
                        if (connection.isMember("unit_id")) {
                            conn.unit_id = connection["unit_id"].asInt();
                        }
                        
                        device.setConnection(conn);
                        break;
                    }
                    case ProtocolType::ECHONET_LITE: {
                        EchonetLiteConnectionConfig conn;
                        
                        if (connection.isMember("ip")) {
                            conn.ip = connection["ip"].asString();
                        }
                        
                        device.setConnection(conn);
                        break;
                    }
                    default:
                        continue;
                }
            }
            
            try {
                if (device.validate()) {
                    devices_.push_back(device);
                }
            } catch (const ConfigValidationError& e) {
                // Log error and skip this device
                continue;
            }
        }
        
        return true;
    } catch (const Json::Exception& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool DeviceConfigCollection::saveToYaml(const std::string& yaml_file) const {
    try {
        YAML::Node devices;
        
        for (const auto& device : devices_) {
            YAML::Node device_node;
            
            // Save ID
            device_node["id"] = device.getId();
            
            // Save template ID
            device_node["template"] = device.getTemplateId();
            
            // Save protocol
            device_node["protocol"] = protocolToString(device.getProtocol());
            
            // Save OCPP ID
            device_node["ocpp_id"] = device.getOcppId();
            
            // Save connection based on protocol
            YAML::Node connection;
            
            switch (device.getProtocol()) {
                case ProtocolType::MODBUS_TCP: {
                    const auto& conn = std::get<ModbusTcpConnectionConfig>(device.getConnection());
                    connection["ip"] = conn.ip;
                    connection["port"] = conn.port;
                    connection["unit_id"] = conn.unit_id;
                    break;
                }
                case ProtocolType::MODBUS_RTU: {
                    const auto& conn = std::get<ModbusRtuConnectionConfig>(device.getConnection());
                    connection["port"] = conn.port;
                    connection["baud_rate"] = conn.baud_rate;
                    connection["data_bits"] = conn.data_bits;
                    connection["stop_bits"] = conn.stop_bits;
                    connection["parity"] = conn.parity;
                    connection["unit_id"] = conn.unit_id;
                    break;
                }
                case ProtocolType::ECHONET_LITE: {
                    const auto& conn = std::get<EchonetLiteConnectionConfig>(device.getConnection());
                    connection["ip"] = conn.ip;
                    break;
                }
                default:
                    continue;
            }
            
            device_node["connection"] = connection;
            
            devices.push_back(device_node);
        }
        
        YAML::Node root;
        root["devices"] = devices;
        
        std::ofstream file(yaml_file);
        if (!file.is_open()) {
            return false;
        }
        
        file << root;
        return true;
    } catch (const YAML::Exception& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool DeviceConfigCollection::saveToJson(const std::string& json_file) const {
    try {
        Json::Value devices(Json::arrayValue);
        
        for (const auto& device : devices_) {
            Json::Value device_node;
            
            // Save ID
            device_node["id"] = device.getId();
            
            // Save template ID
            device_node["template"] = device.getTemplateId();
            
            // Save protocol
            device_node["protocol"] = protocolToString(device.getProtocol());
            
            // Save OCPP ID
            device_node["ocpp_id"] = device.getOcppId();
            
            // Save connection based on protocol
            Json::Value connection;
            
            switch (device.getProtocol()) {
                case ProtocolType::MODBUS_TCP: {
                    const auto& conn = std::get<ModbusTcpConnectionConfig>(device.getConnection());
                    connection["ip"] = conn.ip;
                    connection["port"] = conn.port;
                    connection["unit_id"] = conn.unit_id;
                    break;
                }
                case ProtocolType::MODBUS_RTU: {
                    const auto& conn = std::get<ModbusRtuConnectionConfig>(device.getConnection());
                    connection["port"] = conn.port;
                    connection["baud_rate"] = conn.baud_rate;
                    connection["data_bits"] = conn.data_bits;
                    connection["stop_bits"] = conn.stop_bits;
                    connection["parity"] = conn.parity;
                    connection["unit_id"] = conn.unit_id;
                    break;
                }
                case ProtocolType::ECHONET_LITE: {
                    const auto& conn = std::get<EchonetLiteConnectionConfig>(device.getConnection());
                    connection["ip"] = conn.ip;
                    break;
                }
                default:
                    continue;
            }
            
            device_node["connection"] = connection;
            
            devices.append(device_node);
        }
        
        Json::Value root;
        root["devices"] = devices;
        
        std::ofstream file(json_file);
        if (!file.is_open()) {
            return false;
        }
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        writer->write(root, &file);
        
        return true;
    } catch (const Json::Exception& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool DeviceConfigCollection::validate() const {
    try {
        for (const auto& device : devices_) {
            device.validate();
        }
        
        return true;
    } catch (const ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw ConfigValidationError(std::string("Unexpected error during validation: ") + e.what());
    }
}

void DeviceConfigCollection::addDevice(const DeviceConfig& device) {
    // Check if device with same ID already exists
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if (it->getId() == device.getId()) {
            // Replace existing device
            *it = device;
            return;
        }
    }
    
    // Add new device
    devices_.push_back(device);
}

bool DeviceConfigCollection::removeDevice(const std::string& id) {
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if (it->getId() == id) {
            devices_.erase(it);
            return true;
        }
    }
    
    return false;
}

std::optional<DeviceConfig> DeviceConfigCollection::getDevice(const std::string& id) const {
    // NOLINTNEXTLINE(cppcheck-useStlAlgorithm)
    for (const auto& device : devices_) {
        if (device.getId() == id) {
            return device;
        }
    }
    
    return std::nullopt;
}

} // namespace config
} // namespace ocpp_gateway