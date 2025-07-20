#include "ocpp_gateway/ocpp/mapping_config.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class MappingConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir_ = fs::temp_directory_path() / "mapping_config_test";
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        // Remove test directory
        fs::remove_all(test_dir_);
    }

    fs::path test_dir_;
};

TEST_F(MappingConfigTest, TestModbusVariableMappingValidation) {
    ocpp_gateway::ocpp::ModbusVariableMapping mapping;
    mapping.register_address = 40001;
    mapping.data_type = "uint16";
    mapping.scale = 1.0;
    mapping.unit = "kWh";

    // Valid mapping
    EXPECT_NO_THROW(mapping.validate());

    // Invalid register address
    mapping.register_address = -1;
    EXPECT_THROW(mapping.validate(), ocpp_gateway::config::ConfigValidationError);
    mapping.register_address = 40001;

    // Invalid data type
    mapping.data_type = "invalid";
    EXPECT_THROW(mapping.validate(), ocpp_gateway::config::ConfigValidationError);
    mapping.data_type = "uint16";

    // Invalid scale
    mapping.scale = 0.0;
    EXPECT_THROW(mapping.validate(), ocpp_gateway::config::ConfigValidationError);
    mapping.scale = 1.0;

    // Enum type without enum map
    mapping.data_type = "enum";
    EXPECT_THROW(mapping.validate(), ocpp_gateway::config::ConfigValidationError);
    
    // Valid enum mapping
    mapping.enum_map[0] = "Available";
    mapping.enum_map[1] = "Occupied";
    EXPECT_NO_THROW(mapping.validate());
}

TEST_F(MappingConfigTest, TestEchonetLiteVariableMappingValidation) {
    ocpp_gateway::ocpp::EchonetLiteVariableMapping mapping;
    mapping.epc = 0x80;
    mapping.data_type = "uint8";
    mapping.scale = 1.0;
    mapping.unit = "kWh";

    // Valid mapping
    EXPECT_NO_THROW(mapping.validate());

    // Invalid EPC
    mapping.epc = -1;
    EXPECT_THROW(mapping.validate(), ocpp_gateway::config::ConfigValidationError);
    mapping.epc = 256;
    EXPECT_THROW(mapping.validate(), ocpp_gateway::config::ConfigValidationError);
    mapping.epc = 0x80;

    // Invalid data type
    mapping.data_type = "invalid";
    EXPECT_THROW(mapping.validate(), ocpp_gateway::config::ConfigValidationError);
    mapping.data_type = "uint8";

    // Invalid scale
    mapping.scale = 0.0;
    EXPECT_THROW(mapping.validate(), ocpp_gateway::config::ConfigValidationError);
    mapping.scale = 1.0;

    // Enum type without enum map
    mapping.data_type = "enum";
    EXPECT_THROW(mapping.validate(), ocpp_gateway::config::ConfigValidationError);
    
    // Valid enum mapping
    mapping.enum_map[0] = "Available";
    mapping.enum_map[1] = "Occupied";
    EXPECT_NO_THROW(mapping.validate());
}

TEST_F(MappingConfigTest, TestOcppVariableValidation) {
    ocpp_gateway::ocpp::OcppVariable var;
    var.ocpp_name = "AvailabilityState";
    var.type = "modbus";
    
    ocpp_gateway::ocpp::ModbusVariableMapping modbus_mapping;
    modbus_mapping.register_address = 40001;
    modbus_mapping.data_type = "uint16";
    modbus_mapping.scale = 1.0;
    modbus_mapping.unit = "kWh";
    
    var.mapping = modbus_mapping;

    // Valid variable
    EXPECT_NO_THROW(var.validate());

    // Empty OCPP name
    var.ocpp_name = "";
    EXPECT_THROW(var.validate(), ocpp_gateway::config::ConfigValidationError);
    var.ocpp_name = "AvailabilityState";

    // Invalid type
    var.type = "invalid";
    EXPECT_THROW(var.validate(), ocpp_gateway::config::ConfigValidationError);
    var.type = "modbus";

    // Type mismatch
    var.type = "echonet_lite";
    EXPECT_THROW(var.validate(), ocpp_gateway::config::ConfigValidationError);
    var.type = "modbus";
}

TEST_F(MappingConfigTest, TestMappingTemplateYamlParsing) {
    // Create test YAML file
    std::string yaml_content = R"(
template:
  id: vendor_x_model_y
  description: Vendor X Model Y Charger
  variables:
    - ocpp_name: AvailabilityState
      type: modbus
      register: 40001
      data_type: uint16
      enum:
        0: Available
        1: Occupied
        2: Reserved
        3: Unavailable
        4: Faulted
    - ocpp_name: MeterValue.Energy.Active.Import.Register
      type: modbus
      register: 40010
      data_type: float32
      scale: 0.1
      unit: kWh
)";

    fs::path yaml_file = test_dir_ / "test_template.yaml";
    std::ofstream file(yaml_file);
    file << yaml_content;
    file.close();

    // Parse YAML file
    ocpp_gateway::ocpp::MappingTemplate template_obj;
    EXPECT_NO_THROW(template_obj.loadFromYaml(yaml_file.string()));

    // Verify parsed data
    EXPECT_EQ(template_obj.getId(), "vendor_x_model_y");
    EXPECT_EQ(template_obj.getDescription(), "Vendor X Model Y Charger");
    EXPECT_FALSE(template_obj.hasParent());
    EXPECT_EQ(template_obj.getVariables().size(), 2);

    // Verify first variable
    auto var1 = template_obj.getVariable("AvailabilityState");
    ASSERT_TRUE(var1.has_value());
    EXPECT_EQ(var1->ocpp_name, "AvailabilityState");
    EXPECT_EQ(var1->type, "modbus");
    EXPECT_FALSE(var1->read_only);
    
    ASSERT_TRUE(std::holds_alternative<ocpp_gateway::ocpp::ModbusVariableMapping>(var1->mapping));
    const auto& mapping1 = std::get<ocpp_gateway::ocpp::ModbusVariableMapping>(var1->mapping);
    EXPECT_EQ(mapping1.register_address, 40001);
    EXPECT_EQ(mapping1.data_type, "uint16");
    EXPECT_EQ(mapping1.scale, 1.0);
    EXPECT_TRUE(mapping1.unit.empty());
    EXPECT_EQ(mapping1.enum_map.size(), 5);
    EXPECT_EQ(mapping1.enum_map.at(0), "Available");
    EXPECT_EQ(mapping1.enum_map.at(4), "Faulted");

    // Verify second variable
    auto var2 = template_obj.getVariable("MeterValue.Energy.Active.Import.Register");
    ASSERT_TRUE(var2.has_value());
    EXPECT_EQ(var2->ocpp_name, "MeterValue.Energy.Active.Import.Register");
    EXPECT_EQ(var2->type, "modbus");
    
    ASSERT_TRUE(std::holds_alternative<ocpp_gateway::ocpp::ModbusVariableMapping>(var2->mapping));
    const auto& mapping2 = std::get<ocpp_gateway::ocpp::ModbusVariableMapping>(var2->mapping);
    EXPECT_EQ(mapping2.register_address, 40010);
    EXPECT_EQ(mapping2.data_type, "float32");
    EXPECT_EQ(mapping2.scale, 0.1);
    EXPECT_EQ(mapping2.unit, "kWh");
    EXPECT_TRUE(mapping2.enum_map.empty());
}

TEST_F(MappingConfigTest, TestMappingTemplateJsonParsing) {
    // Create test JSON file
    std::string json_content = R"({
  "template": {
    "id": "vendor_x_model_y",
    "description": "Vendor X Model Y Charger",
    "variables": [
      {
        "ocpp_name": "AvailabilityState",
        "type": "modbus",
        "register": 40001,
        "data_type": "uint16",
        "enum": {
          "0": "Available",
          "1": "Occupied",
          "2": "Reserved",
          "3": "Unavailable",
          "4": "Faulted"
        }
      },
      {
        "ocpp_name": "MeterValue.Energy.Active.Import.Register",
        "type": "modbus",
        "register": 40010,
        "data_type": "float32",
        "scale": 0.1,
        "unit": "kWh"
      }
    ]
  }
})";

    fs::path json_file = test_dir_ / "test_template.json";
    std::ofstream file(json_file);
    file << json_content;
    file.close();

    // Parse JSON file
    ocpp_gateway::ocpp::MappingTemplate template_obj;
    EXPECT_NO_THROW(template_obj.loadFromJson(json_file.string()));

    // Verify parsed data
    EXPECT_EQ(template_obj.getId(), "vendor_x_model_y");
    EXPECT_EQ(template_obj.getDescription(), "Vendor X Model Y Charger");
    EXPECT_FALSE(template_obj.hasParent());
    EXPECT_EQ(template_obj.getVariables().size(), 2);

    // Verify first variable
    auto var1 = template_obj.getVariable("AvailabilityState");
    ASSERT_TRUE(var1.has_value());
    EXPECT_EQ(var1->ocpp_name, "AvailabilityState");
    EXPECT_EQ(var1->type, "modbus");
    
    ASSERT_TRUE(std::holds_alternative<ocpp_gateway::ocpp::ModbusVariableMapping>(var1->mapping));
    const auto& mapping1 = std::get<ocpp_gateway::ocpp::ModbusVariableMapping>(var1->mapping);
    EXPECT_EQ(mapping1.register_address, 40001);
    EXPECT_EQ(mapping1.data_type, "uint16");
    EXPECT_EQ(mapping1.enum_map.size(), 5);
}

TEST_F(MappingConfigTest, TestMappingTemplateInheritance) {
    // Create parent template
    ocpp_gateway::ocpp::MappingTemplate parent_template;
    parent_template.setId("parent_template");
    parent_template.setDescription("Parent Template");
    
    ocpp_gateway::ocpp::OcppVariable parent_var1;
    parent_var1.ocpp_name = "AvailabilityState";
    parent_var1.type = "modbus";
    
    ocpp_gateway::ocpp::ModbusVariableMapping parent_mapping1;
    parent_mapping1.register_address = 40001;
    parent_mapping1.data_type = "uint16";
    parent_mapping1.enum_map[0] = "Available";
    parent_mapping1.enum_map[1] = "Occupied";
    
    parent_var1.mapping = parent_mapping1;
    parent_template.addVariable(parent_var1);
    
    ocpp_gateway::ocpp::OcppVariable parent_var2;
    parent_var2.ocpp_name = "MeterValue.Energy.Active.Import.Register";
    parent_var2.type = "modbus";
    
    ocpp_gateway::ocpp::ModbusVariableMapping parent_mapping2;
    parent_mapping2.register_address = 40010;
    parent_mapping2.data_type = "float32";
    parent_mapping2.scale = 0.1;
    parent_mapping2.unit = "kWh";
    
    parent_var2.mapping = parent_mapping2;
    parent_template.addVariable(parent_var2);
    
    // Create child template
    ocpp_gateway::ocpp::MappingTemplate child_template;
    child_template.setId("child_template");
    child_template.setParentId("parent_template");
    
    // Override one variable
    ocpp_gateway::ocpp::OcppVariable child_var1;
    child_var1.ocpp_name = "AvailabilityState";
    child_var1.type = "modbus";
    child_var1.read_only = true;
    
    ocpp_gateway::ocpp::ModbusVariableMapping child_mapping1;
    child_mapping1.register_address = 40002; // Different register
    child_mapping1.data_type = "uint16";
    child_mapping1.enum_map[0] = "Available";
    child_mapping1.enum_map[1] = "Occupied";
    
    child_var1.mapping = child_mapping1;
    child_template.addVariable(child_var1);
    
    // Add new variable
    ocpp_gateway::ocpp::OcppVariable child_var3;
    child_var3.ocpp_name = "Connector.Status";
    child_var3.type = "modbus";
    
    ocpp_gateway::ocpp::ModbusVariableMapping child_mapping3;
    child_mapping3.register_address = 40003;
    child_mapping3.data_type = "uint16";
    child_mapping3.enum_map[0] = "Available";
    child_mapping3.enum_map[1] = "Occupied";
    
    child_var3.mapping = child_mapping3;
    child_template.addVariable(child_var3);
    
    // Create collection and add templates
    ocpp_gateway::ocpp::MappingTemplateCollection collection;
    collection.addTemplate(parent_template);
    collection.addTemplate(child_template);
    
    // Resolve inheritance
    EXPECT_NO_THROW(collection.resolveInheritance());
    
    // Get resolved child template
    auto resolved_child = collection.getTemplate("child_template");
    ASSERT_TRUE(resolved_child.has_value());
    
    // Verify inheritance
    EXPECT_EQ(resolved_child->getVariables().size(), 3);
    
    // Verify overridden variable
    auto var1 = resolved_child->getVariable("AvailabilityState");
    ASSERT_TRUE(var1.has_value());
    EXPECT_EQ(var1->ocpp_name, "AvailabilityState");
    EXPECT_TRUE(var1->read_only);
    
    ASSERT_TRUE(std::holds_alternative<ocpp_gateway::ocpp::ModbusVariableMapping>(var1->mapping));
    const auto& mapping1 = std::get<ocpp_gateway::ocpp::ModbusVariableMapping>(var1->mapping);
    EXPECT_EQ(mapping1.register_address, 40002); // Child's value
    
    // Verify inherited variable
    auto var2 = resolved_child->getVariable("MeterValue.Energy.Active.Import.Register");
    ASSERT_TRUE(var2.has_value());
    EXPECT_EQ(var2->ocpp_name, "MeterValue.Energy.Active.Import.Register");
    
    ASSERT_TRUE(std::holds_alternative<ocpp_gateway::ocpp::ModbusVariableMapping>(var2->mapping));
    const auto& mapping2 = std::get<ocpp_gateway::ocpp::ModbusVariableMapping>(var2->mapping);
    EXPECT_EQ(mapping2.register_address, 40010); // Parent's value
    EXPECT_EQ(mapping2.scale, 0.1);
    EXPECT_EQ(mapping2.unit, "kWh");
    
    // Verify new variable
    auto var3 = resolved_child->getVariable("Connector.Status");
    ASSERT_TRUE(var3.has_value());
    EXPECT_EQ(var3->ocpp_name, "Connector.Status");
    
    ASSERT_TRUE(std::holds_alternative<ocpp_gateway::ocpp::ModbusVariableMapping>(var3->mapping));
    const auto& mapping3 = std::get<ocpp_gateway::ocpp::ModbusVariableMapping>(var3->mapping);
    EXPECT_EQ(mapping3.register_address, 40003);
}

TEST_F(MappingConfigTest, TestMappingTemplateCollectionDirectoryLoading) {
    // Create test templates
    std::string yaml_content1 = R"(
template:
  id: template1
  description: Template 1
  variables:
    - ocpp_name: AvailabilityState
      type: modbus
      register: 40001
      data_type: uint16
)";

    std::string yaml_content2 = R"(
template:
  id: template2
  parent: template1
  description: Template 2
  variables:
    - ocpp_name: Connector.Status
      type: modbus
      register: 40002
      data_type: uint16
)";

    // Create template directory
    fs::path template_dir = test_dir_ / "templates";
    fs::create_directories(template_dir);
    
    // Write template files
    std::ofstream file1(template_dir / "template1.yaml");
    file1 << yaml_content1;
    file1.close();
    
    std::ofstream file2(template_dir / "template2.yaml");
    file2 << yaml_content2;
    file2.close();
    
    // Load templates from directory
    ocpp_gateway::ocpp::MappingTemplateCollection collection;
    EXPECT_NO_THROW(collection.loadFromDirectory(template_dir.string()));
    
    // Verify loaded templates
    EXPECT_EQ(collection.getTemplates().size(), 2);
    
    auto template1 = collection.getTemplate("template1");
    ASSERT_TRUE(template1.has_value());
    EXPECT_EQ(template1->getId(), "template1");
    EXPECT_EQ(template1->getDescription(), "Template 1");
    EXPECT_EQ(template1->getVariables().size(), 1);
    
    auto template2 = collection.getTemplate("template2");
    ASSERT_TRUE(template2.has_value());
    EXPECT_EQ(template2->getId(), "template2");
    EXPECT_EQ(template2->getDescription(), "Template 2");
    EXPECT_TRUE(template2->hasParent());
    EXPECT_EQ(template2->getParentId().value(), "template1");
    EXPECT_EQ(template2->getVariables().size(), 1);
    
    // Verify inheritance resolution
    EXPECT_NO_THROW(collection.resolveInheritance());
    
    template2 = collection.getTemplate("template2");
    ASSERT_TRUE(template2.has_value());
    EXPECT_FALSE(template2->hasParent());
    EXPECT_EQ(template2->getVariables().size(), 2);
}

TEST_F(MappingConfigTest, TestCircularDependencyDetection) {
    // Create templates with circular dependency
    ocpp_gateway::ocpp::MappingTemplate template1;
    template1.setId("template1");
    template1.setParentId("template2");
    
    ocpp_gateway::ocpp::MappingTemplate template2;
    template2.setId("template2");
    template2.setParentId("template1");
    
    // Create collection and add templates
    ocpp_gateway::ocpp::MappingTemplateCollection collection;
    collection.addTemplate(template1);
    collection.addTemplate(template2);
    
    // Attempt to resolve inheritance
    EXPECT_THROW(collection.resolveInheritance(), ocpp_gateway::config::ConfigValidationError);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}