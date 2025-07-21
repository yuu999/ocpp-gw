#include "ocpp_gateway/ocpp/mapping_config.h"
#include "ocpp_gateway/common/file_watcher.h"
#include "ocpp_gateway/common/logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include <set>
#include <mutex>

namespace ocpp_gateway {
namespace ocpp {

namespace fs = std::filesystem;
using json = nlohmann::json;

// ModbusVariableMapping implementation
bool ModbusVariableMapping::validate() const {
    if (register_address < 0) {
        throw config::ConfigValidationError("Modbus register address must be non-negative");
    }
    
    // Validate data type
    if (data_type != "uint16" && data_type != "int16" && 
        data_type != "uint32" && data_type != "int32" && 
        data_type != "float32" && data_type != "boolean" && 
        data_type != "string" && data_type != "enum") {
        throw config::ConfigValidationError("Invalid Modbus data type: " + data_type);
    }
    
    // Validate scale for numeric types
    if ((data_type == "uint16" || data_type == "int16" || 
         data_type == "uint32" || data_type == "int32" || 
         data_type == "float32") && scale <= 0) {
        throw config::ConfigValidationError("Scale must be positive for numeric data types");
    }
    
    // Validate enum map for enum type
    if (data_type == "enum" && enum_map.empty()) {
        throw config::ConfigValidationError("Enum map cannot be empty for enum data type");
    }
    
    return true;
}
// Echone
tLiteVariableMapping implementation
bool EchonetLiteVariableMapping::validate() const {
    if (epc < 0 || epc > 255) {
        throw config::ConfigValidationError("ECHONET Lite EPC must be between 0 and 255");
    }
    
    // Validate data type
    if (data_type != "uint8" && data_type != "int8" && 
        data_type != "uint16" && data_type != "int16" && 
        data_type != "uint32" && data_type != "int32" && 
        data_type != "float32" && data_type != "boolean" && 
        data_type != "string" && data_type != "enum") {
        throw config::ConfigValidationError("Invalid ECHONET Lite data type: " + data_type);
    }
    
    // Validate scale for numeric types
    if ((data_type == "uint8" || data_type == "int8" || 
         data_type == "uint16" || data_type == "int16" || 
         data_type == "uint32" || data_type == "int32" || 
         data_type == "float32") && scale <= 0) {
        throw config::ConfigValidationError("Scale must be positive for numeric data types");
    }
    
    // Validate enum map for enum type
    if (data_type == "enum" && enum_map.empty()) {
        throw config::ConfigValidationError("Enum map cannot be empty for enum data type");
    }
    
    return true;
}

// OcppVariable implementation
bool OcppVariable::validate() const {
    if (ocpp_name.empty()) {
        throw config::ConfigValidationError("OCPP variable name cannot be empty");
    }
    
    if (type != "modbus" && type != "echonet_lite") {
        throw config::ConfigValidationError("Variable type must be 'modbus' or 'echonet_lite'");
    }
    
    // Validate mapping based on type
    if (type == "modbus") {
        if (!std::holds_alternative<ModbusVariableMapping>(mapping)) {
            throw config::ConfigValidationError("Modbus variable must have Modbus mapping");
        }
        std::get<ModbusVariableMapping>(mapping).validate();
    } else if (type == "echonet_lite") {
        if (!std::holds_alternative<EchonetLiteVariableMapping>(mapping)) {
            throw config::ConfigValidationError("ECHONET Lite variable must have ECHONET Lite mapping");
        }
        std::get<EchonetLiteVariableMapping>(mapping).validate();
    }
    
    return true;
}// M
appingTemplate implementation
MappingTemplate::MappingTemplate()
    : id_(""),
      description_(""),
      parent_id_(std::nullopt) {
}

MappingTemplate::MappingTemplate(
    const std::string& id,
    const std::string& description,
    const std::optional<std::string>& parent_id,
    const std::vector<OcppVariable>& variables)
    : id_(id),
      description_(description),
      parent_id_(parent_id),
      variables_(variables) {
}

bool MappingTemplate::loadFromYaml(const std::string& yaml_file) {
    try {
        YAML::Node root = YAML::LoadFile(yaml_file);
        
        if (!root["template"]) {
            throw config::ConfigValidationError("Missing 'template' section in YAML file");
        }
        
        YAML::Node template_node = root["template"];
        
        // Parse template ID
        if (!template_node["id"]) {
            throw config::ConfigValidationError("Missing 'id' in template");
        }
        id_ = template_node["id"].as<std::string>();
        
        // Parse description (optional)
        if (template_node["description"]) {
            description_ = template_node["description"].as<std::string>();
        }
        
        // Parse parent ID (optional)
        if (template_node["parent"]) {
            parent_id_ = template_node["parent"].as<std::string>();
        } else {
            parent_id_ = std::nullopt;
        }
        
        // Parse variables
        variables_.clear();
        if (template_node["variables"]) {
            YAML::Node variables_node = template_node["variables"];
            if (variables_node.IsSequence()) {
                for (const auto& var_node : variables_node) {
                    OcppVariable var;
                    
                    // Parse OCPP name
                    if (!var_node["ocpp_name"]) {
                        throw config::ConfigValidationError("Missing 'ocpp_name' in variable");
                    }
                    var.ocpp_name = var_node["ocpp_name"].as<std::string>();
                    
                    // Parse type
                    if (!var_node["type"]) {
                        throw config::ConfigValidationError("Missing 'type' in variable");
                    }
                    var.type = var_node["type"].as<std::string>();
                    
                    // Parse read_only (optional)
                    if (var_node["read_only"]) {
                        var.read_only = var_node["read_only"].as<bool>();
                    }   
                 
                    // Parse mapping based on type
                    if (var.type == "modbus") {
                        ModbusVariableMapping modbus_mapping;
                        
                        // Parse register
                        if (!var_node["register"]) {
                            throw config::ConfigValidationError("Missing 'register' in Modbus variable");
                        }
                        modbus_mapping.register_address = var_node["register"].as<int>();
                        
                        // Parse data_type
                        if (!var_node["data_type"]) {
                            throw config::ConfigValidationError("Missing 'data_type' in Modbus variable");
                        }
                        modbus_mapping.data_type = var_node["data_type"].as<std::string>();
                        
                        // Parse scale (optional)
                        if (var_node["scale"]) {
                            modbus_mapping.scale = var_node["scale"].as<double>();
                        }
                        
                        // Parse unit (optional)
                        if (var_node["unit"]) {
                            modbus_mapping.unit = var_node["unit"].as<std::string>();
                        }
                        
                        // Parse enum map (optional)
                        if (var_node["enum"]) {
                            YAML::Node enum_node = var_node["enum"];
                            if (enum_node.IsMap()) {
                                for (const auto& enum_pair : enum_node) {
                                    int key = enum_pair.first.as<int>();
                                    std::string value = enum_pair.second.as<std::string>();
                                    modbus_mapping.enum_map[key] = value;
                                }
                            }
                        }
                        
                        var.mapping = modbus_mapping;
                    } else if (var.type == "echonet_lite") {
                        EchonetLiteVariableMapping el_mapping;
                        
                        // Parse EPC
                        if (!var_node["epc"]) {
                            throw config::ConfigValidationError("Missing 'epc' in ECHONET Lite variable");
                        }
                        el_mapping.epc = var_node["epc"].as<int>();
                        
                        // Parse data_type
                        if (!var_node["data_type"]) {
                            throw config::ConfigValidationError("Missing 'data_type' in ECHONET Lite variable");
                        }
                        el_mapping.data_type = var_node["data_type"].as<std::string>();
                        
                        // Parse scale (optional)
                        if (var_node["scale"]) {
                            el_mapping.scale = var_node["scale"].as<double>();
                        }
                        
                        // Parse unit (optional)
                        if (var_node["unit"]) {
                            el_mapping.unit = var_node["unit"].as<std::string>();
                        }      
                  
                        // Parse enum map (optional)
                        if (var_node["enum"]) {
                            YAML::Node enum_node = var_node["enum"];
                            if (enum_node.IsMap()) {
                                for (const auto& enum_pair : enum_node) {
                                    int key = enum_pair.first.as<int>();
                                    std::string value = enum_pair.second.as<std::string>();
                                    el_mapping.enum_map[key] = value;
                                }
                            }
                        }
                        
                        var.mapping = el_mapping;
                    } else {
                        throw config::ConfigValidationError("Invalid variable type: " + var.type);
                    }
                    
                    variables_.push_back(var);
                }
            }
        }
        
        return true;
    } catch (const YAML::Exception& e) {
        throw config::ConfigValidationError(std::string("YAML parsing error: ") + e.what());
    } catch (const config::ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw config::ConfigValidationError(std::string("Error loading mapping template: ") + e.what());
    }
}

bool MappingTemplate::loadFromYamlString(const std::string& yaml_content) {
    try {
        YAML::Node root = YAML::Load(yaml_content);
        
        if (!root["template"]) {
            throw config::ConfigValidationError("Missing 'template' section in YAML content");
        }
        
        YAML::Node template_node = root["template"];
        
        // Parse template ID
        if (!template_node["id"]) {
            throw config::ConfigValidationError("Missing 'id' in template");
        }
        id_ = template_node["id"].as<std::string>();
        
        // Parse description (optional)
        if (template_node["description"]) {
            description_ = template_node["description"].as<std::string>();
        }
        
        // Parse parent ID (optional)
        if (template_node["parent"]) {
            parent_id_ = template_node["parent"].as<std::string>();
        } else {
            parent_id_ = std::nullopt;
        }       
 
        // Parse variables
        variables_.clear();
        if (template_node["variables"]) {
            YAML::Node variables_node = template_node["variables"];
            if (variables_node.IsSequence()) {
                for (const auto& var_node : variables_node) {
                    OcppVariable var;
                    
                    // Parse OCPP name
                    if (!var_node["ocpp_name"]) {
                        throw config::ConfigValidationError("Missing 'ocpp_name' in variable");
                    }
                    var.ocpp_name = var_node["ocpp_name"].as<std::string>();
                    
                    // Parse type
                    if (!var_node["type"]) {
                        throw config::ConfigValidationError("Missing 'type' in variable");
                    }
                    var.type = var_node["type"].as<std::string>();
                    
                    // Parse read_only (optional)
                    if (var_node["read_only"]) {
                        var.read_only = var_node["read_only"].as<bool>();
                    }
                    
                    // Parse mapping based on type
                    if (var.type == "modbus") {
                        ModbusVariableMapping modbus_mapping;
                        
                        // Parse register
                        if (!var_node["register"]) {
                            throw config::ConfigValidationError("Missing 'register' in Modbus variable");
                        }
                        modbus_mapping.register_address = var_node["register"].as<int>();
                        
                        // Parse data_type
                        if (!var_node["data_type"]) {
                            throw config::ConfigValidationError("Missing 'data_type' in Modbus variable");
                        }
                        modbus_mapping.data_type = var_node["data_type"].as<std::string>();
                        
                        // Parse scale (optional)
                        if (var_node["scale"]) {
                            modbus_mapping.scale = var_node["scale"].as<double>();
                        }
                        
                        // Parse unit (optional)
                        if (var_node["unit"]) {
                            modbus_mapping.unit = var_node["unit"].as<std::string>();
                        }
                        
                        // Parse enum map (optional)
                        if (var_node["enum"]) {
                            YAML::Node enum_node = var_node["enum"];
                            if (enum_node.IsMap()) {
                                for (const auto& enum_pair : enum_node) {
                                    int key = enum_pair.first.as<int>();
                                    std::string value = enum_pair.second.as<std::string>();
                                    modbus_mapping.enum_map[key] = value;
                                }
                            }
                        }      
                  
                        var.mapping = modbus_mapping;
                    } else if (var.type == "echonet_lite") {
                        EchonetLiteVariableMapping el_mapping;
                        
                        // Parse EPC
                        if (!var_node["epc"]) {
                            throw config::ConfigValidationError("Missing 'epc' in ECHONET Lite variable");
                        }
                        el_mapping.epc = var_node["epc"].as<int>();
                        
                        // Parse data_type
                        if (!var_node["data_type"]) {
                            throw config::ConfigValidationError("Missing 'data_type' in ECHONET Lite variable");
                        }
                        el_mapping.data_type = var_node["data_type"].as<std::string>();
                        
                        // Parse scale (optional)
                        if (var_node["scale"]) {
                            el_mapping.scale = var_node["scale"].as<double>();
                        }
                        
                        // Parse unit (optional)
                        if (var_node["unit"]) {
                            el_mapping.unit = var_node["unit"].as<std::string>();
                        }
                        
                        // Parse enum map (optional)
                        if (var_node["enum"]) {
                            YAML::Node enum_node = var_node["enum"];
                            if (enum_node.IsMap()) {
                                for (const auto& enum_pair : enum_node) {
                                    int key = enum_pair.first.as<int>();
                                    std::string value = enum_pair.second.as<std::string>();
                                    el_mapping.enum_map[key] = value;
                                }
                            }
                        }
                        
                        var.mapping = el_mapping;
                    } else {
                        throw config::ConfigValidationError("Invalid variable type: " + var.type);
                    }
                    
                    variables_.push_back(var);
                }
            }
        }
        
        return true;
    } catch (const YAML::Exception& e) {
        throw config::ConfigValidationError(std::string("YAML parsing error: ") + e.what());
    } catch (const config::ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw config::ConfigValidationError(std::string("Error loading mapping template: ") + e.what());
    }
}
bool Mapp
ingTemplate::loadFromJson(const std::string& json_file) {
    try {
        std::ifstream file(json_file);
        if (!file.is_open()) {
            throw config::ConfigValidationError("Failed to open JSON file: " + json_file);
        }
        
        json root;
        file >> root;
        
        if (!root.contains("template")) {
            throw config::ConfigValidationError("Missing 'template' section in JSON file");
        }
        
        json template_node = root["template"];
        
        // Parse template ID
        if (!template_node.contains("id")) {
            throw config::ConfigValidationError("Missing 'id' in template");
        }
        id_ = template_node["id"].get<std::string>();
        
        // Parse description (optional)
        if (template_node.contains("description")) {
            description_ = template_node["description"].get<std::string>();
        }
        
        // Parse parent ID (optional)
        if (template_node.contains("parent")) {
            parent_id_ = template_node["parent"].get<std::string>();
        } else {
            parent_id_ = std::nullopt;
        }
        
        // Parse variables
        variables_.clear();
        if (template_node.contains("variables") && template_node["variables"].is_array()) {
            for (const auto& var_node : template_node["variables"]) {
                OcppVariable var;
                
                // Parse OCPP name
                if (!var_node.contains("ocpp_name")) {
                    throw config::ConfigValidationError("Missing 'ocpp_name' in variable");
                }
                var.ocpp_name = var_node["ocpp_name"].get<std::string>();
                
                // Parse type
                if (!var_node.contains("type")) {
                    throw config::ConfigValidationError("Missing 'type' in variable");
                }
                var.type = var_node["type"].get<std::string>();
                
                // Parse read_only (optional)
                if (var_node.contains("read_only")) {
                    var.read_only = var_node["read_only"].get<bool>();
                } 
               
                // Parse mapping based on type
                if (var.type == "modbus") {
                    ModbusVariableMapping modbus_mapping;
                    
                    // Parse register
                    if (!var_node.contains("register")) {
                        throw config::ConfigValidationError("Missing 'register' in Modbus variable");
                    }
                    modbus_mapping.register_address = var_node["register"].get<int>();
                    
                    // Parse data_type
                    if (!var_node.contains("data_type")) {
                        throw config::ConfigValidationError("Missing 'data_type' in Modbus variable");
                    }
                    modbus_mapping.data_type = var_node["data_type"].get<std::string>();
                    
                    // Parse scale (optional)
                    if (var_node.contains("scale")) {
                        modbus_mapping.scale = var_node["scale"].get<double>();
                    }
                    
                    // Parse unit (optional)
                    if (var_node.contains("unit")) {
                        modbus_mapping.unit = var_node["unit"].get<std::string>();
                    }
                    
                    // Parse enum map (optional)
                    if (var_node.contains("enum") && var_node["enum"].is_object()) {
                        for (auto it = var_node["enum"].begin(); it != var_node["enum"].end(); ++it) {
                            int key = std::stoi(it.key());
                            std::string value = it.value().get<std::string>();
                            modbus_mapping.enum_map[key] = value;
                        }
                    }
                    
                    var.mapping = modbus_mapping;
                } else if (var.type == "echonet_lite") {
                    EchonetLiteVariableMapping el_mapping;
                    
                    // Parse EPC
                    if (!var_node.contains("epc")) {
                        throw config::ConfigValidationError("Missing 'epc' in ECHONET Lite variable");
                    }
                    el_mapping.epc = var_node["epc"].get<int>();
                    
                    // Parse data_type
                    if (!var_node.contains("data_type")) {
                        throw config::ConfigValidationError("Missing 'data_type' in ECHONET Lite variable");
                    }
                    el_mapping.data_type = var_node["data_type"].get<std::string>();
                    
                    // Parse scale (optional)
                    if (var_node.contains("scale")) {
                        el_mapping.scale = var_node["scale"].get<double>();
                    }
                    
                    // Parse unit (optional)
                    if (var_node.contains("unit")) {
                        el_mapping.unit = var_node["unit"].get<std::string>();
                    } 
                   
                    // Parse enum map (optional)
                    if (var_node.contains("enum") && var_node["enum"].is_object()) {
                        for (auto it = var_node["enum"].begin(); it != var_node["enum"].end(); ++it) {
                            int key = std::stoi(it.key());
                            std::string value = it.value().get<std::string>();
                            el_mapping.enum_map[key] = value;
                        }
                    }
                    
                    var.mapping = el_mapping;
                } else {
                    throw config::ConfigValidationError("Invalid variable type: " + var.type);
                }
                
                variables_.push_back(var);
            }
        }
        
        return true;
    } catch (const json::exception& e) {
        throw config::ConfigValidationError(std::string("JSON parsing error: ") + e.what());
    } catch (const config::ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw config::ConfigValidationError(std::string("Error loading mapping template: ") + e.what());
    }
}

bool MappingTemplate::loadFromJsonString(const std::string& json_content) {
    try {
        json root = json::parse(json_content);
        
        if (!root.contains("template")) {
            throw config::ConfigValidationError("Missing 'template' section in JSON content");
        }
        
        json template_node = root["template"];
        
        // Parse template ID
        if (!template_node.contains("id")) {
            throw config::ConfigValidationError("Missing 'id' in template");
        }
        id_ = template_node["id"].get<std::string>();
        
        // Parse description (optional)
        if (template_node.contains("description")) {
            description_ = template_node["description"].get<std::string>();
        }
        
        // Parse parent ID (optional)
        if (template_node.contains("parent")) {
            parent_id_ = template_node["parent"].get<std::string>();
        } else {
            parent_id_ = std::nullopt;
        }     
   
        // Parse variables
        variables_.clear();
        if (template_node.contains("variables") && template_node["variables"].is_array()) {
            for (const auto& var_node : template_node["variables"]) {
                OcppVariable var;
                
                // Parse OCPP name
                if (!var_node.contains("ocpp_name")) {
                    throw config::ConfigValidationError("Missing 'ocpp_name' in variable");
                }
                var.ocpp_name = var_node["ocpp_name"].get<std::string>();
                
                // Parse type
                if (!var_node.contains("type")) {
                    throw config::ConfigValidationError("Missing 'type' in variable");
                }
                var.type = var_node["type"].get<std::string>();
                
                // Parse read_only (optional)
                if (var_node.contains("read_only")) {
                    var.read_only = var_node["read_only"].get<bool>();
                }
                
                // Parse mapping based on type
                if (var.type == "modbus") {
                    ModbusVariableMapping modbus_mapping;
                    
                    // Parse register
                    if (!var_node.contains("register")) {
                        throw config::ConfigValidationError("Missing 'register' in Modbus variable");
                    }
                    modbus_mapping.register_address = var_node["register"].get<int>();
                    
                    // Parse data_type
                    if (!var_node.contains("data_type")) {
                        throw config::ConfigValidationError("Missing 'data_type' in Modbus variable");
                    }
                    modbus_mapping.data_type = var_node["data_type"].get<std::string>();
                    
                    // Parse scale (optional)
                    if (var_node.contains("scale")) {
                        modbus_mapping.scale = var_node["scale"].get<double>();
                    }
                    
                    // Parse unit (optional)
                    if (var_node.contains("unit")) {
                        modbus_mapping.unit = var_node["unit"].get<std::string>();
                    }
                    
                    // Parse enum map (optional)
                    if (var_node.contains("enum") && var_node["enum"].is_object()) {
                        for (auto it = var_node["enum"].begin(); it != var_node["enum"].end(); ++it) {
                            int key = std::stoi(it.key());
                            std::string value = it.value().get<std::string>();
                            modbus_mapping.enum_map[key] = value;
                        }
                    } 
                   
                    var.mapping = modbus_mapping;
                } else if (var.type == "echonet_lite") {
                    EchonetLiteVariableMapping el_mapping;
                    
                    // Parse EPC
                    if (!var_node.contains("epc")) {
                        throw config::ConfigValidationError("Missing 'epc' in ECHONET Lite variable");
                    }
                    el_mapping.epc = var_node["epc"].get<int>();
                    
                    // Parse data_type
                    if (!var_node.contains("data_type")) {
                        throw config::ConfigValidationError("Missing 'data_type' in ECHONET Lite variable");
                    }
                    el_mapping.data_type = var_node["data_type"].get<std::string>();
                    
                    // Parse scale (optional)
                    if (var_node.contains("scale")) {
                        el_mapping.scale = var_node["scale"].get<double>();
                    }
                    
                    // Parse unit (optional)
                    if (var_node.contains("unit")) {
                        el_mapping.unit = var_node["unit"].get<std::string>();
                    }
                    
                    // Parse enum map (optional)
                    if (var_node.contains("enum") && var_node["enum"].is_object()) {
                        for (auto it = var_node["enum"].begin(); it != var_node["enum"].end(); ++it) {
                            int key = std::stoi(it.key());
                            std::string value = it.value().get<std::string>();
                            el_mapping.enum_map[key] = value;
                        }
                    }
                    
                    var.mapping = el_mapping;
                } else {
                    throw config::ConfigValidationError("Invalid variable type: " + var.type);
                }
                
                variables_.push_back(var);
            }
        }
        
        return true;
    } catch (const json::exception& e) {
        throw config::ConfigValidationError(std::string("JSON parsing error: ") + e.what());
    } catch (const config::ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw config::ConfigValidationError(std::string("Error loading mapping template: ") + e.what());
    }
}bo
ol MappingTemplate::saveToYaml(const std::string& yaml_file) const {
    try {
        YAML::Node root;
        YAML::Node template_node;
        
        // Add template ID
        template_node["id"] = id_;
        
        // Add description
        if (!description_.empty()) {
            template_node["description"] = description_;
        }
        
        // Add parent ID
        if (parent_id_.has_value()) {
            template_node["parent"] = parent_id_.value();
        }
        
        // Add variables
        YAML::Node variables_node;
        for (const auto& var : variables_) {
            YAML::Node var_node;
            
            // Add OCPP name
            var_node["ocpp_name"] = var.ocpp_name;
            
            // Add type
            var_node["type"] = var.type;
            
            // Add read_only
            if (var.read_only) {
                var_node["read_only"] = var.read_only;
            }
            
            // Add mapping based on type
            if (var.type == "modbus") {
                const auto& modbus_mapping = std::get<ModbusVariableMapping>(var.mapping);
                
                // Add register
                var_node["register"] = modbus_mapping.register_address;
                
                // Add data_type
                var_node["data_type"] = modbus_mapping.data_type;
                
                // Add scale
                if (modbus_mapping.scale != 1.0) {
                    var_node["scale"] = modbus_mapping.scale;
                }
                
                // Add unit
                if (!modbus_mapping.unit.empty()) {
                    var_node["unit"] = modbus_mapping.unit;
                }
                
                // Add enum map
                if (!modbus_mapping.enum_map.empty()) {
                    YAML::Node enum_node;
                    for (const auto& enum_pair : modbus_mapping.enum_map) {
                        enum_node[enum_pair.first] = enum_pair.second;
                    }
                    var_node["enum"] = enum_node;
                }
            } else if (var.type == "echonet_lite") {
                const auto& el_mapping = std::get<EchonetLiteVariableMapping>(var.mapping);          
      
                // Add EPC
                var_node["epc"] = el_mapping.epc;
                
                // Add data_type
                var_node["data_type"] = el_mapping.data_type;
                
                // Add scale
                if (el_mapping.scale != 1.0) {
                    var_node["scale"] = el_mapping.scale;
                }
                
                // Add unit
                if (!el_mapping.unit.empty()) {
                    var_node["unit"] = el_mapping.unit;
                }
                
                // Add enum map
                if (!el_mapping.enum_map.empty()) {
                    YAML::Node enum_node;
                    for (const auto& enum_pair : el_mapping.enum_map) {
                        enum_node[enum_pair.first] = enum_pair.second;
                    }
                    var_node["enum"] = enum_node;
                }
            }
            
            variables_node.push_back(var_node);
        }
        
        template_node["variables"] = variables_node;
        root["template"] = template_node;
        
        // Create directory if it doesn't exist
        fs::path file_path(yaml_file);
        fs::create_directories(file_path.parent_path());
        
        // Save to file
        std::ofstream file(yaml_file);
        if (!file.is_open()) {
            throw config::ConfigValidationError("Failed to open file for writing: " + yaml_file);
        }
        
        file << YAML::Dump(root);
        
        return true;
    } catch (const YAML::Exception& e) {
        throw config::ConfigValidationError(std::string("YAML error: ") + e.what());
    } catch (const std::exception& e) {
        throw config::ConfigValidationError(std::string("Error saving mapping template: ") + e.what());
    }
}

bool MappingTemplate::saveToJson(const std::string& json_file) const {
    try {
        json root;
        json template_node;
        
        // Add template ID
        template_node["id"] = id_;
        
        // Add description
        if (!description_.empty()) {
            template_node["description"] = description_;
        }    
    
        // Add parent ID
        if (parent_id_.has_value()) {
            template_node["parent"] = parent_id_.value();
        }
        
        // Add variables
        json variables_array = json::array();
        for (const auto& var : variables_) {
            json var_node;
            
            // Add OCPP name
            var_node["ocpp_name"] = var.ocpp_name;
            
            // Add type
            var_node["type"] = var.type;
            
            // Add read_only
            if (var.read_only) {
                var_node["read_only"] = var.read_only;
            }
            
            // Add mapping based on type
            if (var.type == "modbus") {
                const auto& modbus_mapping = std::get<ModbusVariableMapping>(var.mapping);
                
                // Add register
                var_node["register"] = modbus_mapping.register_address;
                
                // Add data_type
                var_node["data_type"] = modbus_mapping.data_type;
                
                // Add scale
                if (modbus_mapping.scale != 1.0) {
                    var_node["scale"] = modbus_mapping.scale;
                }
                
                // Add unit
                if (!modbus_mapping.unit.empty()) {
                    var_node["unit"] = modbus_mapping.unit;
                }
                
                // Add enum map
                if (!modbus_mapping.enum_map.empty()) {
                    json enum_node;
                    for (const auto& enum_pair : modbus_mapping.enum_map) {
                        enum_node[std::to_string(enum_pair.first)] = enum_pair.second;
                    }
                    var_node["enum"] = enum_node;
                }
            } else if (var.type == "echonet_lite") {
                const auto& el_mapping = std::get<EchonetLiteVariableMapping>(var.mapping);
                
                // Add EPC
                var_node["epc"] = el_mapping.epc;
                
                // Add data_type
                var_node["data_type"] = el_mapping.data_type;
                
                // Add scale
                if (el_mapping.scale != 1.0) {
                    var_node["scale"] = el_mapping.scale;
                } 
               
                // Add unit
                if (!el_mapping.unit.empty()) {
                    var_node["unit"] = el_mapping.unit;
                }
                
                // Add enum map
                if (!el_mapping.enum_map.empty()) {
                    json enum_node;
                    for (const auto& enum_pair : el_mapping.enum_map) {
                        enum_node[std::to_string(enum_pair.first)] = enum_pair.second;
                    }
                    var_node["enum"] = enum_node;
                }
            }
            
            variables_array.push_back(var_node);
        }
        
        template_node["variables"] = variables_array;
        root["template"] = template_node;
        
        // Create directory if it doesn't exist
        fs::path file_path(json_file);
        fs::create_directories(file_path.parent_path());
        
        // Save to file
        std::ofstream file(json_file);
        if (!file.is_open()) {
            throw config::ConfigValidationError("Failed to open file for writing: " + json_file);
        }
        
        file << root.dump(4); // Pretty print with 4-space indentation
        
        return true;
    } catch (const json::exception& e) {
        throw config::ConfigValidationError(std::string("JSON error: ") + e.what());
    } catch (const std::exception& e) {
        throw config::ConfigValidationError(std::string("Error saving mapping template: ") + e.what());
    }
}

bool MappingTemplate::validate() const {
    try {
        // Validate template ID
        if (id_.empty()) {
            throw config::ConfigValidationError("Template ID cannot be empty");
        }
        
        // Validate variables
        for (const auto& var : variables_) {
            var.validate();
        }
        
        // Check for duplicate OCPP names
        std::set<std::string> ocpp_names;
        for (const auto& var : variables_) {
            if (ocpp_names.find(var.ocpp_name) != ocpp_names.end()) {
                throw config::ConfigValidationError("Duplicate OCPP variable name: " + var.ocpp_name);
            }
            ocpp_names.insert(var.ocpp_name);
        } 
       
        return true;
    } catch (const config::ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw config::ConfigValidationError(std::string("Error validating mapping template: ") + e.what());
    }
}

void MappingTemplate::applyParent(const MappingTemplate& parent) {
    // Copy description if empty
    if (description_.empty()) {
        description_ = parent.description_;
    }
    
    // Merge variables
    std::map<std::string, OcppVariable> var_map;
    
    // Add parent variables first
    for (const auto& var : parent.variables_) {
        var_map[var.ocpp_name] = var;
    }
    
    // Override with child variables
    for (const auto& var : variables_) {
        var_map[var.ocpp_name] = var;
    }
    
    // Update variables list
    variables_.clear();
    for (const auto& pair : var_map) {
        variables_.push_back(pair.second);
    }
}

void MappingTemplate::addVariable(const OcppVariable& variable) {
    // Check if variable already exists
    for (auto it = variables_.begin(); it != variables_.end(); ++it) {
        if (it->ocpp_name == variable.ocpp_name) {
            // Replace existing variable
            *it = variable;
            return;
        }
    }
    
    // Add new variable
    variables_.push_back(variable);
}

std::optional<OcppVariable> MappingTemplate::getVariable(const std::string& ocpp_name) const {
    for (const auto& var : variables_) {
        if (var.ocpp_name == ocpp_name) {
            return var;
        }
    }
    
    return std::nullopt;
}//
 MappingTemplateCollection implementation
MappingTemplateCollection::MappingTemplateCollection() {
}

bool MappingTemplateCollection::loadFromDirectory(const std::string& directory) {
    try {
        if (!fs::exists(directory)) {
            throw config::ConfigValidationError("Directory does not exist: " + directory);
        }
        
        if (!fs::is_directory(directory)) {
            throw config::ConfigValidationError("Path is not a directory: " + directory);
        }
        
        // Clear existing templates
        templates_.clear();
        
        // Load all YAML and JSON files in the directory
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string file_path = entry.path().string();
                std::string extension = entry.path().extension().string();
                
                if (extension == ".yaml" || extension == ".yml" || extension == ".json") {
                    loadFromFile(file_path);
                }
            }
        }
        
        // Resolve template inheritance
        resolveInheritance();
        
        return true;
    } catch (const config::ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw config::ConfigValidationError(std::string("Error loading mapping templates from directory: ") + e.what());
    }
}

bool MappingTemplateCollection::loadFromFile(const std::string& file_path) {
    try {
        MappingTemplate template_obj;
        std::string extension = fs::path(file_path).extension().string();
        
        if (extension == ".yaml" || extension == ".yml") {
            template_obj.loadFromYaml(file_path);
        } else if (extension == ".json") {
            template_obj.loadFromJson(file_path);
        } else {
            throw config::ConfigValidationError("Unsupported file extension: " + extension);
        }
        
        // Add template to collection
        addTemplate(template_obj);
        
        return true;
    } catch (const config::ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw config::ConfigValidationError(std::string("Error loading mapping template from file: ") + e.what());
    }
}bo
ol MappingTemplateCollection::saveToDirectory(const std::string& directory) const {
    try {
        // Create directory if it doesn't exist
        fs::create_directories(directory);
        
        // Save each template to a separate file
        for (const auto& template_obj : templates_) {
            std::string file_path = directory + "/" + template_obj.getId() + ".yaml";
            template_obj.saveToYaml(file_path);
        }
        
        return true;
    } catch (const config::ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw config::ConfigValidationError(std::string("Error saving mapping templates to directory: ") + e.what());
    }
}

bool MappingTemplateCollection::validate() const {
    try {
        // Validate each template
        for (const auto& template_obj : templates_) {
            template_obj.validate();
        }
        
        // Check for duplicate template IDs
        std::set<std::string> template_ids;
        for (const auto& template_obj : templates_) {
            if (template_ids.find(template_obj.getId()) != template_ids.end()) {
                throw config::ConfigValidationError("Duplicate template ID: " + template_obj.getId());
            }
            template_ids.insert(template_obj.getId());
        }
        
        return true;
    } catch (const config::ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw config::ConfigValidationError(std::string("Error validating mapping templates: ") + e.what());
    }
}

bool MappingTemplateCollection::resolveInheritance() {
    try {
        // Check for circular dependencies
        std::map<std::string, std::set<std::string>> dependency_graph;
        
        // Build dependency graph
        for (const auto& template_obj : templates_) {
            if (template_obj.hasParent()) {
                dependency_graph[template_obj.getId()].insert(template_obj.getParentId().value());
            }
        } 
       
        // Check for circular dependencies
        for (const auto& pair : dependency_graph) {
            std::set<std::string> visited;
            std::set<std::string> path;
            
            std::function<bool(const std::string&)> has_cycle = [&](const std::string& node) -> bool {
                if (visited.find(node) != visited.end()) {
                    return false;
                }
                
                visited.insert(node);
                path.insert(node);
                
                if (dependency_graph.find(node) != dependency_graph.end()) {
                    for (const auto& neighbor : dependency_graph[node]) {
                        if (path.find(neighbor) != path.end()) {
                            return true; // Cycle detected
                        }
                        
                        if (has_cycle(neighbor)) {
                            return true;
                        }
                    }
                }
                
                path.erase(node);
                return false;
            };
            
            if (has_cycle(pair.first)) {
                throw config::ConfigValidationError("Circular dependency detected in template inheritance");
            }
        }
        
        // Apply inheritance
        bool changes_made;
        do {
            changes_made = false;
            
            for (auto& template_obj : templates_) {
                if (template_obj.hasParent()) {
                    std::string parent_id = template_obj.getParentId().value();
                    auto parent = getTemplate(parent_id);
                    
                    if (!parent) {
                        throw config::ConfigValidationError("Parent template not found: " + parent_id);
                    }
                    
                    // Apply parent template
                    template_obj.applyParent(parent.value());
                    
                    // Clear parent reference after applying
                    template_obj.setParentId(std::nullopt);
                    changes_made = true;
                }
            }
        } while (changes_made);
        
        return true;
    } catch (const config::ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw config::ConfigValidationError(std::string("Error resolving template inheritance: ") + e.what());
    }
}void Ma
ppingTemplateCollection::addTemplate(const MappingTemplate& template_obj) {
    // Check if template already exists
    for (auto it = templates_.begin(); it != templates_.end(); ++it) {
        if (it->getId() == template_obj.getId()) {
            // Replace existing template
            *it = template_obj;
            return;
        }
    }
    
    // Add new template
    templates_.push_back(template_obj);
}

bool MappingTemplateCollection::removeTemplate(const std::string& id) {
    for (auto it = templates_.begin(); it != templates_.end(); ++it) {
        if (it->getId() == id) {
            templates_.erase(it);
            return true;
        }
    }
    
    return false;
}

std::optional<MappingTemplate> MappingTemplateCollection::getTemplate(const std::string& id) const {
    for (const auto& template_obj : templates_) {
        if (template_obj.getId() == id) {
            return template_obj;
        }
    }
    
    return std::nullopt;
}

} // namespace ocpp
} // namespace ocpp_gateway

// MappingTemplateCollection implementation for hot reload functionality
MappingTemplateCollection::~MappingTemplateCollection() {
    disableHotReload();
}

bool MappingTemplateCollection::enableHotReload(const std::string& directory, TemplateChangeCallback callback) {
    if (hot_reload_enabled_) {
        // Already enabled, disable first
        disableHotReload();
    }

    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        LOG_ERROR("Cannot enable hot reload: directory {} does not exist", directory);
        return false;
    }

    try {
        watched_directory_ = directory;
        file_watcher_ = std::make_shared<common::FileWatcher>();
        
        if (callback) {
            registerChangeCallback(callback);
        }

        // Add directory watch for YAML and JSON files
        bool yaml_watch = file_watcher_->addDirectoryWatch(
            directory,
            [this](const std::string& file_path) {
                this->handleFileChange(file_path);
            },
            ".yaml",
            false
        );

        bool json_watch = file_watcher_->addDirectoryWatch(
            directory,
            [this](const std::string& file_path) {
                this->handleFileChange(file_path);
            },
            ".json",
            false
        );

        if (!yaml_watch && !json_watch) {
            LOG_ERROR("Failed to set up directory watches for hot reload");
            file_watcher_.reset();
            return false;
        }

        // Start the file watcher
        file_watcher_->start();
        hot_reload_enabled_ = true;
        LOG_INFO("Hot reload enabled for directory: {}", directory);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error enabling hot reload: {}", e.what());
        file_watcher_.reset();
        return false;
    }
}

void MappingTemplateCollection::disableHotReload() {
    if (!hot_reload_enabled_) {
        return;
    }

    if (file_watcher_) {
        file_watcher_->stop();
        file_watcher_.reset();
    }

    hot_reload_enabled_ = false;
    watched_directory_.clear();
    LOG_INFO("Hot reload disabled");
}

void MappingTemplateCollection::registerChangeCallback(TemplateChangeCallback callback) {
    if (callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        change_callbacks_.push_back(callback);
    }
}

void MappingTemplateCollection::clearChangeCallbacks() {
    std::lock_guard<std::mutex> lock(mutex_);
    change_callbacks_.clear();
}

void MappingTemplateCollection::handleFileChange(const std::string& file_path) {
    LOG_INFO("Detected change in configuration file: {}", file_path);
    
    try {
        // Create a copy of the current templates for atomic update
        std::vector<MappingTemplate> old_templates = templates_;
        
        // Try to load the changed file
        if (!loadFromFile(file_path)) {
            LOG_ERROR("Failed to load changed file: {}", file_path);
            return;
        }
        
        // Validate the templates
        try {
            validate();
        } catch (const config::ConfigValidationError& e) {
            LOG_ERROR("Validation failed for changed file {}: {}", file_path, e.what());
            // Restore old templates
            templates_ = old_templates;
            return;
        }
        
        // Try to resolve inheritance
        try {
            resolveInheritance();
        } catch (const config::ConfigValidationError& e) {
            LOG_ERROR("Inheritance resolution failed for changed file {}: {}", file_path, e.what());
            // Restore old templates
            templates_ = old_templates;
            return;
        }
        
        LOG_INFO("Successfully reloaded configuration from file: {}", file_path);
        
        // Notify callbacks
        std::vector<TemplateChangeCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callbacks = change_callbacks_;
        }
        
        for (const auto& callback : callbacks) {
            try {
                callback(file_path);
            } catch (const std::exception& e) {
                LOG_ERROR("Error in template change callback: {}", e.what());
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error handling file change for {}: {}", file_path, e.what());
    }
}

bool MappingTemplateCollection::reloadTemplates() {
    if (watched_directory_.empty()) {
        LOG_ERROR("Cannot reload templates: no watched directory set");
        return false;
    }
    
    try {
        // Create a copy of the current templates for atomic update
        std::vector<MappingTemplate> old_templates = templates_;
        
        // Clear current templates
        templates_.clear();
        
        // Load templates from directory
        if (!loadFromDirectory(watched_directory_)) {
            LOG_ERROR("Failed to reload templates from directory: {}", watched_directory_);
            // Restore old templates
            templates_ = old_templates;
            return false;
        }
        
        // Validate the templates
        try {
            validate();
        } catch (const config::ConfigValidationError& e) {
            LOG_ERROR("Validation failed for reloaded templates: {}", e.what());
            // Restore old templates
            templates_ = old_templates;
            return false;
        }
        
        // Try to resolve inheritance
        try {
            resolveInheritance();
        } catch (const config::ConfigValidationError& e) {
            LOG_ERROR("Inheritance resolution failed for reloaded templates: {}", e.what());
            // Restore old templates
            templates_ = old_templates;
            return false;
        }
        
        LOG_INFO("Successfully reloaded all templates from directory: {}", watched_directory_);
        
        // Notify callbacks
        std::vector<TemplateChangeCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callbacks = change_callbacks_;
        }
        
        for (const auto& callback : callbacks) {
            try {
                callback(watched_directory_);
            } catch (const std::exception& e) {
                LOG_ERROR("Error in template change callback: {}", e.what());
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error reloading templates: {}", e.what());
        return false;
    }
}