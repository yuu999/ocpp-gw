#include "ocpp_gateway/ocpp/variable_translator.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace ocpp_gateway {
namespace ocpp {

VariableTranslator::VariableTranslator(const MappingTemplate& mapping_template)
    : mapping_template_(mapping_template) {
}

DeviceData VariableTranslator::translateToDevice(const std::string& ocpp_name, const OcppValue& value) const {
    // Find the variable in the mapping template
    auto var_opt = mapping_template_.getVariable(ocpp_name);
    if (!var_opt.has_value()) {
        throw TranslationError("Variable not found in mapping template: " + ocpp_name);
    }

    const auto& var = var_opt.value();
    
    // Check if variable is read-only
    if (var.read_only) {
        throw TranslationError("Cannot write to read-only variable: " + ocpp_name);
    }

    // Translate based on variable type
    if (var.type == "modbus") {
        if (!std::holds_alternative<ModbusVariableMapping>(var.mapping)) {
            throw TranslationError("Modbus variable has incorrect mapping type: " + ocpp_name);
        }
        const auto& mapping = std::get<ModbusVariableMapping>(var.mapping);
        ModbusData modbus_data = convertOcppToModbusData(mapping, value);
        return modbus_data;
    } else if (var.type == "echonet_lite") {
        if (!std::holds_alternative<EchonetLiteVariableMapping>(var.mapping)) {
            throw TranslationError("ECHONET Lite variable has incorrect mapping type: " + ocpp_name);
        }
        const auto& mapping = std::get<EchonetLiteVariableMapping>(var.mapping);
        EchonetLiteData el_data = convertOcppToEchonetLiteData(mapping, value);
        return el_data;
    } else {
        throw TranslationError("Unsupported variable type: " + var.type);
    }
}

OcppValue VariableTranslator::translateToOcpp(const std::string& ocpp_name, const DeviceData& data) const {
    // Find the variable in the mapping template
    auto var_opt = mapping_template_.getVariable(ocpp_name);
    if (!var_opt.has_value()) {
        throw TranslationError("Variable not found in mapping template: " + ocpp_name);
    }

    const auto& var = var_opt.value();

    // Translate based on variable type
    if (var.type == "modbus") {
        if (!std::holds_alternative<ModbusVariableMapping>(var.mapping)) {
            throw TranslationError("Modbus variable has incorrect mapping type: " + ocpp_name);
        }
        const auto& mapping = std::get<ModbusVariableMapping>(var.mapping);
        
        if (!std::holds_alternative<ModbusData>(data)) {
            throw TranslationError("Expected Modbus data for variable: " + ocpp_name);
        }
        const auto& modbus_data = std::get<ModbusData>(data);
        
        return convertModbusDataToOcpp(mapping, modbus_data);
    } else if (var.type == "echonet_lite") {
        if (!std::holds_alternative<EchonetLiteVariableMapping>(var.mapping)) {
            throw TranslationError("ECHONET Lite variable has incorrect mapping type: " + ocpp_name);
        }
        const auto& mapping = std::get<EchonetLiteVariableMapping>(var.mapping);
        
        if (!std::holds_alternative<EchonetLiteData>(data)) {
            throw TranslationError("Expected ECHONET Lite data for variable: " + ocpp_name);
        }
        const auto& el_data = std::get<EchonetLiteData>(data);
        
        return convertEchonetLiteDataToOcpp(mapping, el_data);
    } else {
        throw TranslationError("Unsupported variable type: " + var.type);
    }
}

OcppValue VariableTranslator::convertModbusDataToOcpp(const ModbusVariableMapping& mapping, const ModbusData& data) const {
    // Check if data is large enough
    size_t required_size = 0;
    if (mapping.data_type == "uint16" || mapping.data_type == "int16") {
        required_size = 2;
    } else if (mapping.data_type == "uint32" || mapping.data_type == "int32" || mapping.data_type == "float32") {
        required_size = 4;
    } else if (mapping.data_type == "boolean") {
        required_size = 1;
    } else if (mapping.data_type == "string") {
        // String can be of variable length
        required_size = 1;
    } else if (mapping.data_type == "enum") {
        required_size = 2; // Assuming enum is stored as uint16
    } else {
        throw TranslationError("Unsupported Modbus data type: " + mapping.data_type);
    }

    if (data.data.size() < required_size) {
        throw TranslationError("Modbus data too small for data type: " + mapping.data_type);
    }

    // Convert based on data type
    if (mapping.data_type == "uint16") {
        uint16_t value = (static_cast<uint16_t>(data.data[0]) << 8) | data.data[1];
        double scaled_value = applyScaling(value, mapping.scale, false);
        return static_cast<int>(scaled_value);
    } else if (mapping.data_type == "int16") {
        int16_t value = (static_cast<int16_t>(data.data[0]) << 8) | data.data[1];
        double scaled_value = applyScaling(value, mapping.scale, false);
        return static_cast<int>(scaled_value);
    } else if (mapping.data_type == "uint32") {
        uint32_t value = (static_cast<uint32_t>(data.data[0]) << 24) |
                         (static_cast<uint32_t>(data.data[1]) << 16) |
                         (static_cast<uint32_t>(data.data[2]) << 8) |
                         data.data[3];
        double scaled_value = applyScaling(value, mapping.scale, false);
        return static_cast<int>(scaled_value);
    } else if (mapping.data_type == "int32") {
        int32_t value = (static_cast<int32_t>(data.data[0]) << 24) |
                        (static_cast<int32_t>(data.data[1]) << 16) |
                        (static_cast<int32_t>(data.data[2]) << 8) |
                        data.data[3];
        double scaled_value = applyScaling(value, mapping.scale, false);
        return static_cast<int>(scaled_value);
    } else if (mapping.data_type == "float32") {
        // IEEE 754 float conversion
        uint32_t bits = (static_cast<uint32_t>(data.data[0]) << 24) |
                        (static_cast<uint32_t>(data.data[1]) << 16) |
                        (static_cast<uint32_t>(data.data[2]) << 8) |
                        data.data[3];
        float value;
        std::memcpy(&value, &bits, sizeof(float));
        double scaled_value = applyScaling(value, mapping.scale, false);
        return scaled_value;
    } else if (mapping.data_type == "boolean") {
        return data.data[0] != 0;
    } else if (mapping.data_type == "string") {
        return std::string(data.data.begin(), data.data.end());
    } else if (mapping.data_type == "enum") {
        uint16_t value = (static_cast<uint16_t>(data.data[0]) << 8) | data.data[1];
        return mapEnumToString(mapping.enum_map, value);
    } else {
        throw TranslationError("Unsupported Modbus data type: " + mapping.data_type);
    }
}

OcppValue VariableTranslator::convertEchonetLiteDataToOcpp(const EchonetLiteVariableMapping& mapping, const EchonetLiteData& data) const {
    // Check if data is large enough
    size_t required_size = 0;
    if (mapping.data_type == "uint8" || mapping.data_type == "int8" || mapping.data_type == "boolean") {
        required_size = 1;
    } else if (mapping.data_type == "uint16" || mapping.data_type == "int16") {
        required_size = 2;
    } else if (mapping.data_type == "uint32" || mapping.data_type == "int32" || mapping.data_type == "float32") {
        required_size = 4;
    } else if (mapping.data_type == "string") {
        // String can be of variable length
        required_size = 1;
    } else if (mapping.data_type == "enum") {
        required_size = 1; // Assuming enum is stored as uint8 for ECHONET Lite
    } else {
        throw TranslationError("Unsupported ECHONET Lite data type: " + mapping.data_type);
    }

    if (data.data.size() < required_size) {
        throw TranslationError("ECHONET Lite data too small for data type: " + mapping.data_type);
    }

    // Convert based on data type
    if (mapping.data_type == "uint8") {
        uint8_t value = data.data[0];
        double scaled_value = applyScaling(value, mapping.scale, false);
        return static_cast<int>(scaled_value);
    } else if (mapping.data_type == "int8") {
        int8_t value = static_cast<int8_t>(data.data[0]);
        double scaled_value = applyScaling(value, mapping.scale, false);
        return static_cast<int>(scaled_value);
    } else if (mapping.data_type == "uint16") {
        uint16_t value = (static_cast<uint16_t>(data.data[0]) << 8) | data.data[1];
        double scaled_value = applyScaling(value, mapping.scale, false);
        return static_cast<int>(scaled_value);
    } else if (mapping.data_type == "int16") {
        int16_t value = (static_cast<int16_t>(data.data[0]) << 8) | data.data[1];
        double scaled_value = applyScaling(value, mapping.scale, false);
        return static_cast<int>(scaled_value);
    } else if (mapping.data_type == "uint32") {
        uint32_t value = (static_cast<uint32_t>(data.data[0]) << 24) |
                         (static_cast<uint32_t>(data.data[1]) << 16) |
                         (static_cast<uint32_t>(data.data[2]) << 8) |
                         data.data[3];
        double scaled_value = applyScaling(value, mapping.scale, false);
        return static_cast<int>(scaled_value);
    } else if (mapping.data_type == "int32") {
        int32_t value = (static_cast<int32_t>(data.data[0]) << 24) |
                        (static_cast<int32_t>(data.data[1]) << 16) |
                        (static_cast<int32_t>(data.data[2]) << 8) |
                        data.data[3];
        double scaled_value = applyScaling(value, mapping.scale, false);
        return static_cast<int>(scaled_value);
    } else if (mapping.data_type == "float32") {
        // IEEE 754 float conversion
        uint32_t bits = (static_cast<uint32_t>(data.data[0]) << 24) |
                        (static_cast<uint32_t>(data.data[1]) << 16) |
                        (static_cast<uint32_t>(data.data[2]) << 8) |
                        data.data[3];
        float value;
        std::memcpy(&value, &bits, sizeof(float));
        double scaled_value = applyScaling(value, mapping.scale, false);
        return scaled_value;
    } else if (mapping.data_type == "boolean") {
        return data.data[0] != 0;
    } else if (mapping.data_type == "string") {
        return std::string(data.data.begin(), data.data.end());
    } else if (mapping.data_type == "enum") {
        uint8_t value = data.data[0];
        return mapEnumToString(mapping.enum_map, value);
    } else {
        throw TranslationError("Unsupported ECHONET Lite data type: " + mapping.data_type);
    }
}

ModbusData VariableTranslator::convertOcppToModbusData(const ModbusVariableMapping& mapping, const OcppValue& value) const {
    ModbusData result;

    // Convert based on data type
    if (mapping.data_type == "uint16") {
        double numeric_value = 0.0;
        if (std::holds_alternative<int>(value)) {
            numeric_value = static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            numeric_value = std::get<double>(value);
        } else {
            throw TranslationError("Expected numeric value for uint16 data type");
        }
        
        double scaled_value = applyScaling(numeric_value, mapping.scale, true);
        uint16_t uint_value = static_cast<uint16_t>(std::round(scaled_value));
        
        result.data.push_back(static_cast<uint8_t>(uint_value >> 8));
        result.data.push_back(static_cast<uint8_t>(uint_value & 0xFF));
    } else if (mapping.data_type == "int16") {
        double numeric_value = 0.0;
        if (std::holds_alternative<int>(value)) {
            numeric_value = static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            numeric_value = std::get<double>(value);
        } else {
            throw TranslationError("Expected numeric value for int16 data type");
        }
        
        double scaled_value = applyScaling(numeric_value, mapping.scale, true);
        int16_t int_value = static_cast<int16_t>(std::round(scaled_value));
        
        result.data.push_back(static_cast<uint8_t>(int_value >> 8));
        result.data.push_back(static_cast<uint8_t>(int_value & 0xFF));
    } else if (mapping.data_type == "uint32") {
        double numeric_value = 0.0;
        if (std::holds_alternative<int>(value)) {
            numeric_value = static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            numeric_value = std::get<double>(value);
        } else {
            throw TranslationError("Expected numeric value for uint32 data type");
        }
        
        double scaled_value = applyScaling(numeric_value, mapping.scale, true);
        uint32_t uint_value = static_cast<uint32_t>(std::round(scaled_value));
        
        result.data.push_back(static_cast<uint8_t>(uint_value >> 24));
        result.data.push_back(static_cast<uint8_t>((uint_value >> 16) & 0xFF));
        result.data.push_back(static_cast<uint8_t>((uint_value >> 8) & 0xFF));
        result.data.push_back(static_cast<uint8_t>(uint_value & 0xFF));
    } else if (mapping.data_type == "int32") {
        double numeric_value = 0.0;
        if (std::holds_alternative<int>(value)) {
            numeric_value = static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            numeric_value = std::get<double>(value);
        } else {
            throw TranslationError("Expected numeric value for int32 data type");
        }
        
        double scaled_value = applyScaling(numeric_value, mapping.scale, true);
        int32_t int_value = static_cast<int32_t>(std::round(scaled_value));
        
        result.data.push_back(static_cast<uint8_t>(int_value >> 24));
        result.data.push_back(static_cast<uint8_t>((int_value >> 16) & 0xFF));
        result.data.push_back(static_cast<uint8_t>((int_value >> 8) & 0xFF));
        result.data.push_back(static_cast<uint8_t>(int_value & 0xFF));
    } else if (mapping.data_type == "float32") {
        double numeric_value = 0.0;
        if (std::holds_alternative<int>(value)) {
            numeric_value = static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            numeric_value = std::get<double>(value);
        } else {
            throw TranslationError("Expected numeric value for float32 data type");
        }
        
        double scaled_value = applyScaling(numeric_value, mapping.scale, true);
        float float_value = static_cast<float>(scaled_value);
        
        uint32_t bits;
        std::memcpy(&bits, &float_value, sizeof(float));
        
        result.data.push_back(static_cast<uint8_t>(bits >> 24));
        result.data.push_back(static_cast<uint8_t>((bits >> 16) & 0xFF));
        result.data.push_back(static_cast<uint8_t>((bits >> 8) & 0xFF));
        result.data.push_back(static_cast<uint8_t>(bits & 0xFF));
    } else if (mapping.data_type == "boolean") {
        if (!std::holds_alternative<bool>(value)) {
            throw TranslationError("Expected boolean value for boolean data type");
        }
        bool bool_value = std::get<bool>(value);
        result.data.push_back(bool_value ? 1 : 0);
    } else if (mapping.data_type == "string") {
        if (!std::holds_alternative<std::string>(value)) {
            throw TranslationError("Expected string value for string data type");
        }
        const std::string& str_value = std::get<std::string>(value);
        result.data.assign(str_value.begin(), str_value.end());
    } else if (mapping.data_type == "enum") {
        if (!std::holds_alternative<std::string>(value)) {
            throw TranslationError("Expected string value for enum data type");
        }
        const std::string& str_value = std::get<std::string>(value);
        int int_value = mapEnumToInt(mapping.enum_map, str_value);
        
        result.data.push_back(static_cast<uint8_t>(int_value >> 8));
        result.data.push_back(static_cast<uint8_t>(int_value & 0xFF));
    } else {
        throw TranslationError("Unsupported Modbus data type: " + mapping.data_type);
    }

    return result;
}

EchonetLiteData VariableTranslator::convertOcppToEchonetLiteData(const EchonetLiteVariableMapping& mapping, const OcppValue& value) const {
    EchonetLiteData result;

    // Convert based on data type
    if (mapping.data_type == "uint8") {
        double numeric_value = 0.0;
        if (std::holds_alternative<int>(value)) {
            numeric_value = static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            numeric_value = std::get<double>(value);
        } else {
            throw TranslationError("Expected numeric value for uint8 data type");
        }
        
        double scaled_value = applyScaling(numeric_value, mapping.scale, true);
        uint8_t uint_value = static_cast<uint8_t>(std::round(scaled_value));
        
        result.data.push_back(uint_value);
    } else if (mapping.data_type == "int8") {
        double numeric_value = 0.0;
        if (std::holds_alternative<int>(value)) {
            numeric_value = static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            numeric_value = std::get<double>(value);
        } else {
            throw TranslationError("Expected numeric value for int8 data type");
        }
        
        double scaled_value = applyScaling(numeric_value, mapping.scale, true);
        int8_t int_value = static_cast<int8_t>(std::round(scaled_value));
        
        result.data.push_back(static_cast<uint8_t>(int_value));
    } else if (mapping.data_type == "uint16") {
        double numeric_value = 0.0;
        if (std::holds_alternative<int>(value)) {
            numeric_value = static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            numeric_value = std::get<double>(value);
        } else {
            throw TranslationError("Expected numeric value for uint16 data type");
        }
        
        double scaled_value = applyScaling(numeric_value, mapping.scale, true);
        uint16_t uint_value = static_cast<uint16_t>(std::round(scaled_value));
        
        result.data.push_back(static_cast<uint8_t>(uint_value >> 8));
        result.data.push_back(static_cast<uint8_t>(uint_value & 0xFF));
    } else if (mapping.data_type == "int16") {
        double numeric_value = 0.0;
        if (std::holds_alternative<int>(value)) {
            numeric_value = static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            numeric_value = std::get<double>(value);
        } else {
            throw TranslationError("Expected numeric value for int16 data type");
        }
        
        double scaled_value = applyScaling(numeric_value, mapping.scale, true);
        int16_t int_value = static_cast<int16_t>(std::round(scaled_value));
        
        result.data.push_back(static_cast<uint8_t>(int_value >> 8));
        result.data.push_back(static_cast<uint8_t>(int_value & 0xFF));
    } else if (mapping.data_type == "uint32") {
        double numeric_value = 0.0;
        if (std::holds_alternative<int>(value)) {
            numeric_value = static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            numeric_value = std::get<double>(value);
        } else {
            throw TranslationError("Expected numeric value for uint32 data type");
        }
        
        double scaled_value = applyScaling(numeric_value, mapping.scale, true);
        uint32_t uint_value = static_cast<uint32_t>(std::round(scaled_value));
        
        result.data.push_back(static_cast<uint8_t>(uint_value >> 24));
        result.data.push_back(static_cast<uint8_t>((uint_value >> 16) & 0xFF));
        result.data.push_back(static_cast<uint8_t>((uint_value >> 8) & 0xFF));
        result.data.push_back(static_cast<uint8_t>(uint_value & 0xFF));
    } else if (mapping.data_type == "int32") {
        double numeric_value = 0.0;
        if (std::holds_alternative<int>(value)) {
            numeric_value = static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            numeric_value = std::get<double>(value);
        } else {
            throw TranslationError("Expected numeric value for int32 data type");
        }
        
        double scaled_value = applyScaling(numeric_value, mapping.scale, true);
        int32_t int_value = static_cast<int32_t>(std::round(scaled_value));
        
        result.data.push_back(static_cast<uint8_t>(int_value >> 24));
        result.data.push_back(static_cast<uint8_t>((int_value >> 16) & 0xFF));
        result.data.push_back(static_cast<uint8_t>((int_value >> 8) & 0xFF));
        result.data.push_back(static_cast<uint8_t>(int_value & 0xFF));
    } else if (mapping.data_type == "float32") {
        double numeric_value = 0.0;
        if (std::holds_alternative<int>(value)) {
            numeric_value = static_cast<double>(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            numeric_value = std::get<double>(value);
        } else {
            throw TranslationError("Expected numeric value for float32 data type");
        }
        
        double scaled_value = applyScaling(numeric_value, mapping.scale, true);
        float float_value = static_cast<float>(scaled_value);
        
        uint32_t bits;
        std::memcpy(&bits, &float_value, sizeof(float));
        
        result.data.push_back(static_cast<uint8_t>(bits >> 24));
        result.data.push_back(static_cast<uint8_t>((bits >> 16) & 0xFF));
        result.data.push_back(static_cast<uint8_t>((bits >> 8) & 0xFF));
        result.data.push_back(static_cast<uint8_t>(bits & 0xFF));
    } else if (mapping.data_type == "boolean") {
        if (!std::holds_alternative<bool>(value)) {
            throw TranslationError("Expected boolean value for boolean data type");
        }
        bool bool_value = std::get<bool>(value);
        result.data.push_back(bool_value ? 1 : 0);
    } else if (mapping.data_type == "string") {
        if (!std::holds_alternative<std::string>(value)) {
            throw TranslationError("Expected string value for string data type");
        }
        const std::string& str_value = std::get<std::string>(value);
        result.data.assign(str_value.begin(), str_value.end());
    } else if (mapping.data_type == "enum") {
        if (!std::holds_alternative<std::string>(value)) {
            throw TranslationError("Expected string value for enum data type");
        }
        const std::string& str_value = std::get<std::string>(value);
        int int_value = mapEnumToInt(mapping.enum_map, str_value);
        
        result.data.push_back(static_cast<uint8_t>(int_value));
    } else {
        throw TranslationError("Unsupported ECHONET Lite data type: " + mapping.data_type);
    }

    return result;
}

double VariableTranslator::applyScaling(double value, double scale, bool to_device) const {
    if (to_device) {
        // When converting to device, divide by scale
        return value / scale;
    } else {
        // When converting to OCPP, multiply by scale
        return value * scale;
    }
}

std::string VariableTranslator::mapEnumToString(const std::map<int, std::string>& enum_map, int value) const {
    auto it = enum_map.find(value);
    if (it == enum_map.end()) {
        throw TranslationError("Enum value not found in mapping: " + std::to_string(value));
    }
    return it->second;
}

int VariableTranslator::mapEnumToInt(const std::map<int, std::string>& enum_map, const std::string& value) const {
    for (const auto& pair : enum_map) {
        if (pair.second == value) {
            return pair.first;
        }
    }
    throw TranslationError("Enum string not found in mapping: " + value);
}

} // namespace ocpp
} // namespace ocpp_gateway