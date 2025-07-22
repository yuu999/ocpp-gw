#pragma once

#include "ocpp_gateway/common/config_types.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <variant>

namespace ocpp_gateway {
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
    /**
     * @brief Construct a new Mapping Template Collection object
     */
    MappingTemplateCollection();

    /**
     * @brief Destroy the Mapping Template Collection object
     */
    ~MappingTemplateCollection();

    /**
     * @brief Load mapping templates from directory
     * 
     * @param directory Directory containing mapping template files
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromDirectory(const std::string& directory);

    /**
     * @brief Load a mapping template from file
     * 
     * @param file_path Path to mapping template file
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadFromFile(const std::string& file_path);

    /**
     * @brief Save all mapping templates to directory
     * 
     * @param directory Directory to save mapping templates
     * @return true if saving was successful
     * @return false if saving failed
     */
    bool saveToDirectory(const std::string& directory) const;

    /**
     * @brief Validate all mapping templates
     * 
     * @return true if all templates are valid
     * @throws config::ConfigValidationError if any template is invalid
     */
    bool validate() const;

    /**
     * @brief Resolve template inheritance
     * 
     * This method resolves template inheritance by applying parent templates to child templates.
     * It throws an exception if there are circular dependencies or missing parent templates.
     * 
     * @return true if inheritance resolution was successful
     * @throws config::ConfigValidationError if inheritance resolution failed
     */
    bool resolveInheritance();

    /**
     * @brief Add a mapping template
     * 
     * @param template_obj Mapping template
     */
    void addTemplate(const MappingTemplate& template_obj);

    /**
     * @brief Remove a mapping template by ID
     * 
     * @param id Template ID
     * @return true if template was removed
     * @return false if template was not found
     */
    bool removeTemplate(const std::string& id);

    /**
     * @brief Get a mapping template by ID
     * 
     * @param id Template ID
     * @return std::optional<MappingTemplate> Template if found, empty optional otherwise
     */
    std::optional<MappingTemplate> getTemplate(const std::string& id) const;

    /**
     * @brief Get all mapping templates
     * 
     * @return const std::vector<MappingTemplate>& Vector of mapping templates
     */
    const std::vector<MappingTemplate>& getTemplates() const { return templates_; }

    /**
     * @brief Enable hot reload for mapping templates
     * 
     * @param directory Directory to watch for changes
     * @param callback Optional callback to be notified when templates are reloaded
     * @return true if hot reload was enabled successfully
     * @return false if hot reload could not be enabled
     */
    bool enableHotReload(const std::string& directory, TemplateChangeCallback callback = nullptr);

    /**
     * @brief Disable hot reload for mapping templates
     */
    void disableHotReload();

    /**
     * @brief Check if hot reload is enabled
     * 
     * @return true if hot reload is enabled
     * @return false if hot reload is disabled
     */
    bool isHotReloadEnabled() const { return hot_reload_enabled_; }

    /**
     * @brief Get the directory being watched for changes
     * 
     * @return std::string Directory path or empty string if hot reload is disabled
     */
    std::string getWatchedDirectory() const { return watched_directory_; }

    /**
     * @brief Register a callback for template change notifications
     * 
     * @param callback Callback function to be called when templates are reloaded
     */
    void registerChangeCallback(TemplateChangeCallback callback);

    /**
     * @brief Clear all registered callbacks
     */
    void clearChangeCallbacks();

private:
    std::vector<MappingTemplate> templates_;
    std::string watched_directory_;
    bool hot_reload_enabled_ = false;
    std::shared_ptr<common::FileWatcher> file_watcher_;
    std::vector<TemplateChangeCallback> change_callbacks_;
    std::mutex mutex_;

    /**
     * @brief Handle file change event
     * 
     * @param file_path Path to the changed file
     */
    void handleFileChange(const std::string& file_path);

    /**
     * @brief Reload templates from the watched directory
     * 
     * @return true if reload was successful
     * @return false if reload failed
     */
    bool reloadTemplates();
};

} // namespace ocpp
} // namespace ocpp_gateway