#pragma once

#include "ocpp_gateway/common/config_types.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <functional>
#include <mutex>

#include <nlohmann/json.hpp>

namespace ocpp_gateway {
namespace common {
class FileWatcher;
}
namespace ocpp {

/**
 * @brief Enumeration for variable data types
 */
enum class VariableDataType {
    BOOLEAN,
    INTEGER,
    DECIMAL,
    STRING,
    DATETIME,
    ENUM
};

/**
 * @brief Convert string to variable data type
 * 
 * @param type Data type string
 * @return VariableDataType Corresponding data type
 */
inline VariableDataType dataTypeFromString(const std::string& type) {
    if (type == "boolean") {
        return VariableDataType::BOOLEAN;
    } else if (type == "integer" || type == "int" || type == "uint16" || type == "int32" || type == "uint32") {
        return VariableDataType::INTEGER;
    } else if (type == "decimal" || type == "float" || type == "float32" || type == "double") {
        return VariableDataType::DECIMAL;
    } else if (type == "string") {
        return VariableDataType::STRING;
    } else if (type == "datetime") {
        return VariableDataType::DATETIME;
    } else if (type == "enum") {
        return VariableDataType::ENUM;
    }
    return VariableDataType::STRING; // Default
}

/**
 * @brief Convert variable data type to string
 * 
 * @param type Data type
 * @return std::string Corresponding data type string
 */
inline std::string dataTypeToString(VariableDataType type) {
    switch (type) {
        case VariableDataType::BOOLEAN:
            return "boolean";
        case VariableDataType::INTEGER:
            return "integer";
        case VariableDataType::DECIMAL:
            return "decimal";
        case VariableDataType::STRING:
            return "string";
        case VariableDataType::DATETIME:
            return "datetime";
        case VariableDataType::ENUM:
            return "enum";
        default:
            return "string";
    }
}

/**
 * @brief Modbus register variable mapping
 */
struct ModbusVariableMapping {
    int register_address;
    std::string data_type;
    double scale = 1.0;
    std::string unit;
    std::map<int, std::string> enum_map;

    bool validate() const;
};

/**
 * @brief ECHONET Lite EPC variable mapping
 */
struct EchonetLiteVariableMapping {
    int epc;
    std::string data_type;
    double scale = 1.0;
    std::string unit;
    std::map<int, std::string> enum_map;

    bool validate() const;
};

/**
 * @brief Variable mapping variant for different protocols
 */
using VariableMapping = std::variant<
    ModbusVariableMapping,
    EchonetLiteVariableMapping
>;

/**
 * @brief OCPP variable configuration
 */
struct OcppVariable {
    std::string ocpp_name;
    std::string type; // "modbus" or "echonet_lite"
    VariableMapping mapping;
    bool read_only = false;

    bool validate() const;
};

/**
 * @brief Mapping template configuration
 */
class MappingTemplate {
public:
    /**
     * @brief Construct a new Mapping Template object with default values
     */
    MappingTemplate();

    /**
     * @brief Construct a new Mapping Template object with specified values
     * 
     * @param id Template ID
     * @param description Template description
     * @param parent_id Parent template ID (optional)
     * @param variables OCPP variables
     */
    MappingTemplate(
        const std::string& id,
        const std::string& description,
        const std::optional<std::string>& parent_id,
        const std::vector<OcppVariable>& variables
    );

    /**
     * @brief Load mapping template from YAML file
     * 
     * @param yaml_file Path to YAML configuration file
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromYaml(const std::string& yaml_file);

    /**
     * @brief Load mapping template from YAML string
     * 
     * @param yaml_content YAML content as string
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromYamlString(const std::string& yaml_content);

    /**
     * @brief Load mapping template from JSON file
     * 
     * @param json_file Path to JSON configuration file
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromJson(const std::string& json_file);

    /**
     * @brief Load mapping template from JSON string
     * 
     * @param json_content JSON content as string
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromJsonString(const std::string& json_content);

    /**
     * @brief Save mapping template to YAML file
     * 
     * @param yaml_file Path to YAML configuration file
     * @return true if saving was successful
     * @return false if saving failed
     */
    bool saveToYaml(const std::string& yaml_file) const;

    /**
     * @brief Save mapping template to JSON file
     * 
     * @param json_file Path to JSON configuration file
     * @return true if saving was successful
     * @return false if saving failed
     */
    bool saveToJson(const std::string& json_file) const;

    /**
     * @brief Validate the mapping template
     * 
     * @return true if template is valid
     * @throws config::ConfigValidationError if template is invalid
     */
    bool validate() const;

    /**
     * @brief Apply parent template inheritance
     * 
     * @param parent Parent template
     */
    void applyParent(const MappingTemplate& parent);

    /**
     * @brief Check if template has a parent
     * 
     * @return true if template has a parent
     * @return false if template does not have a parent
     */
    bool hasParent() const { return parent_id_.has_value(); }

    // Getters and setters
    const std::string& getId() const { return id_; }
    void setId(const std::string& id) { id_ = id; }

    const std::string& getDescription() const { return description_; }
    void setDescription(const std::string& description) { description_ = description; }

    const std::optional<std::string>& getParentId() const { return parent_id_; }
    void setParentId(const std::optional<std::string>& parent_id) { parent_id_ = parent_id; }

    const std::vector<OcppVariable>& getVariables() const { return variables_; }
    void setVariables(const std::vector<OcppVariable>& variables) { variables_ = variables; }

    /**
     * @brief Add a variable to the template
     * 
     * @param variable OCPP variable
     */
    void addVariable(const OcppVariable& variable);

    /**
     * @brief Get a variable by OCPP name
     * 
     * @param ocpp_name OCPP variable name
     * @return std::optional<OcppVariable> Variable if found, empty optional otherwise
     */
    std::optional<OcppVariable> getVariable(const std::string& ocpp_name) const;

private:
    std::string id_;
    std::string description_;
    std::optional<std::string> parent_id_;
    std::vector<OcppVariable> variables_;
};

/**
 * @brief Callback function type for template change notifications
 */
using TemplateChangeCallback = std::function<void(const std::string&)>;

/**
 * @brief Mapping template collection class
 */
class MappingTemplateCollection {
public:
    using TemplateChangeCallback = std::function<void(const std::string&)>;

    MappingTemplateCollection();
    ~MappingTemplateCollection();

    void addTemplate(const MappingTemplate& template_obj);
    const MappingTemplate* findTemplate(const std::string& name) const;

    bool loadFromDirectory(const std::string& directory);
    bool loadFromFile(const std::string& file_path);
    void validate();
    void resolveInheritance();
    void clear();

    bool enableHotReload(const std::string& directory, TemplateChangeCallback callback = nullptr);
    void disableHotReload();

    void registerChangeCallback(TemplateChangeCallback callback);
    void clearChangeCallbacks();

private:
    void handleFileChange(const std::string& file_path);
    bool reloadTemplates();

    std::vector<MappingTemplate> templates_;
    std::map<std::string, int> template_name_to_idx_;
    std::map<std::string, std::vector<std::string>> inheritance_graph_;

    std::shared_ptr<common::FileWatcher> file_watcher_;
    std::vector<TemplateChangeCallback> change_callbacks_;
    std::mutex mutex_;
    bool hot_reload_enabled_ = false;
    std::string watched_directory_;
};

} // namespace ocpp
} // namespace ocpp_gateway