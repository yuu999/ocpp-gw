#pragma once

#include "ocpp_gateway/ocpp/mapping_config.h"
#include "ocpp_gateway/common/error.h"
#include <string>
#include <variant>
#include <vector>
#include <map>
#include <memory>
#include <optional>

namespace ocpp_gateway {
namespace ocpp {

/**
 * @brief Exception class for variable translation errors
 */
class TranslationError : public common::RuntimeError {
public:
    explicit TranslationError(const std::string& message) 
        : common::RuntimeError("Translation error: " + message) {}
};

/**
 * @brief Value type for OCPP variables
 */
using OcppValue = std::variant<bool, int, double, std::string>;

/**
 * @brief Raw device data for Modbus
 */
struct ModbusData {
    std::vector<uint8_t> data;
};

/**
 * @brief Raw device data for ECHONET Lite
 */
struct EchonetLiteData {
    std::vector<uint8_t> data;
};

/**
 * @brief Raw device data variant
 */
using DeviceData = std::variant<ModbusData, EchonetLiteData>;

/**
 * @brief Class for translating between OCPP variables and device registers/EPCs
 */
class VariableTranslator {
public:
    /**
     * @brief Construct a new Variable Translator object
     * 
     * @param mapping_template Mapping template to use for translation
     */
    explicit VariableTranslator(const MappingTemplate& mapping_template);

    /**
     * @brief Translate OCPP value to device data
     * 
     * @param ocpp_name OCPP variable name
     * @param value OCPP value
     * @return DeviceData Device data
     * @throws TranslationError if translation fails
     */
    DeviceData translateToDevice(const std::string& ocpp_name, const OcppValue& value) const;

    /**
     * @brief Translate device data to OCPP value
     * 
     * @param ocpp_name OCPP variable name
     * @param data Device data
     * @return OcppValue OCPP value
     * @throws TranslationError if translation fails
     */
    OcppValue translateToOcpp(const std::string& ocpp_name, const DeviceData& data) const;

    /**
     * @brief Get the mapping template
     * 
     * @return const MappingTemplate& Mapping template
     */
    const MappingTemplate& getMappingTemplate() const __attribute__((unused)) { return mapping_template_; }

private:
    /**
     * @brief Convert Modbus data to OCPP value
     * 
     * @param mapping Modbus variable mapping
     * @param data Modbus data
     * @return OcppValue OCPP value
     * @throws TranslationError if conversion fails
     */
    OcppValue convertModbusDataToOcpp(const ModbusVariableMapping& mapping, const ModbusData& data) const;

    /**
     * @brief Convert ECHONET Lite data to OCPP value
     * 
     * @param mapping ECHONET Lite variable mapping
     * @param data ECHONET Lite data
     * @return OcppValue OCPP value
     * @throws TranslationError if conversion fails
     */
    OcppValue convertEchonetLiteDataToOcpp(const EchonetLiteVariableMapping& mapping, const EchonetLiteData& data) const;

    /**
     * @brief Convert OCPP value to Modbus data
     * 
     * @param mapping Modbus variable mapping
     * @param value OCPP value
     * @return ModbusData Modbus data
     * @throws TranslationError if conversion fails
     */
    ModbusData convertOcppToModbusData(const ModbusVariableMapping& mapping, const OcppValue& value) const;

    /**
     * @brief Convert OCPP value to ECHONET Lite data
     * 
     * @param mapping ECHONET Lite variable mapping
     * @param value OCPP value
     * @return EchonetLiteData ECHONET Lite data
     * @throws TranslationError if conversion fails
     */
    EchonetLiteData convertOcppToEchonetLiteData(const EchonetLiteVariableMapping& mapping, const OcppValue& value) const;

    /**
     * @brief Apply scaling to numeric value
     * 
     * @param value Numeric value
     * @param scale Scale factor
     * @param to_device Whether scaling is for device (true) or OCPP (false)
     * @return double Scaled value
     */
    double applyScaling(double value, double scale, bool to_device) const;

    /**
     * @brief Map enum value from integer to string
     * 
     * @param enum_map Enum mapping
     * @param value Integer value
     * @return std::string String value
     * @throws TranslationError if mapping fails
     */
    std::string mapEnumToString(const std::map<int, std::string>& enum_map, int value) const;

    /**
     * @brief Map enum value from string to integer
     * 
     * @param enum_map Enum mapping
     * @param value String value
     * @return int Integer value
     * @throws TranslationError if mapping fails
     */
    int mapEnumToInt(const std::map<int, std::string>& enum_map, const std::string& value) const;

    MappingTemplate mapping_template_;
};

} // namespace ocpp
} // namespace ocpp_gateway