#include "ocpp_gateway/ocpp/variable_translator.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class VariableTranslatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir_ = fs::temp_directory_path() / "variable_translator_test";
        fs::create_directories(test_dir_);
        
        // Create test mapping template
        createTestMappingTemplate();
    }

    void TearDown() override {
        // Remove test directory
        fs::remove_all(test_dir_);
    }

    void createTestMappingTemplate() {
        // Create a mapping template with various data types
        template_.setId("test_template");
        template_.setDescription("Test Template");
        
        // Add Modbus variables
        ocpp_gateway::ocpp::OcppVariable var1;
        var1.ocpp_name = "AvailabilityState";
        var1.type = "modbus";
        
        ocpp_gateway::ocpp::ModbusVariableMapping mapping1;
        mapping1.register_address = 40001;
        mapping1.data_type = "uint16";
        mapping1.enum_map[0] = "Available";
        mapping1.enum_map[1] = "Occupied";
        mapping1.enum_map[2] = "Reserved";
        mapping1.enum_map[3] = "Unavailable";
        mapping1.enum_map[4] = "Faulted";
        
        var1.mapping = mapping1;
        template_.addVariable(var1);
        
        ocpp_gateway::ocpp::OcppVariable var2;
        var2.ocpp_name = "MeterValue.Energy.Active.Import.Register";
        var2.type = "modbus";
        
        ocpp_gateway::ocpp::ModbusVariableMapping mapping2;
        mapping2.register_address = 40010;
        mapping2.data_type = "float32";
        mapping2.scale = 0.1;
        mapping2.unit = "kWh";
        
        var2.mapping = mapping2;
        template_.addVariable(var2);
        
        ocpp_gateway::ocpp::OcppVariable var3;
        var3.ocpp_name = "Connector.Status";
        var3.type = "modbus";
        var3.read_only = true;
        
        ocpp_gateway::ocpp::ModbusVariableMapping mapping3;
        mapping3.register_address = 40020;
        mapping3.data_type = "boolean";
        
        var3.mapping = mapping3;
        template_.addVariable(var3);
        
        // Add ECHONET Lite variables
        ocpp_gateway::ocpp::OcppVariable var4;
        var4.ocpp_name = "ChargingState";
        var4.type = "echonet_lite";
        
        ocpp_gateway::ocpp::EchonetLiteVariableMapping mapping4;
        mapping4.epc = 0x80;
        mapping4.data_type = "uint8";
        mapping4.enum_map[0] = "Idle";
        mapping4.enum_map[1] = "Charging";
        mapping4.enum_map[2] = "SuspendedEV";
        mapping4.enum_map[3] = "SuspendedEVSE";
        mapping4.enum_map[4] = "Finishing";
        
        var4.mapping = mapping4;
        template_.addVariable(var4);
        
        ocpp_gateway::ocpp::OcppVariable var5;
        var5.ocpp_name = "Current.Import";
        var5.type = "echonet_lite";
        
        ocpp_gateway::ocpp::EchonetLiteVariableMapping mapping5;
        mapping5.epc = 0xE0;
        mapping5.data_type = "int16";
        mapping5.scale = 0.1;
        mapping5.unit = "A";
        
        var5.mapping = mapping5;
        template_.addVariable(var5);
    }

    fs::path test_dir_;
    ocpp_gateway::ocpp::MappingTemplate template_;
};

TEST_F(VariableTranslatorTest, TestModbusEnumTranslation) {
    ocpp_gateway::ocpp::VariableTranslator translator(template_);
    
    // Test OCPP to Modbus translation for enum
    ocpp_gateway::ocpp::OcppValue ocpp_value = std::string("Available");
    auto device_data = translator.translateToDevice("AvailabilityState", ocpp_value);
    
    ASSERT_TRUE(std::holds_alternative<ocpp_gateway::ocpp::ModbusData>(device_data));
    const auto& modbus_data = std::get<ocpp_gateway::ocpp::ModbusData>(device_data);
    
    ASSERT_EQ(modbus_data.data.size(), 2);
    EXPECT_EQ(modbus_data.data[0], 0);
    EXPECT_EQ(modbus_data.data[1], 0);
    
    // Test Modbus to OCPP translation for enum
    ocpp_gateway::ocpp::ModbusData test_data;
    test_data.data = {0, 1}; // Value 1 = "Occupied"
    
    ocpp_gateway::ocpp::DeviceData input_data = test_data;
    auto result = translator.translateToOcpp("AvailabilityState", input_data);
    
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    EXPECT_EQ(std::get<std::string>(result), "Occupied");
}

TEST_F(VariableTranslatorTest, TestModbusFloatWithScalingTranslation) {
    ocpp_gateway::ocpp::VariableTranslator translator(template_);
    
    // Test OCPP to Modbus translation for float with scaling
    ocpp_gateway::ocpp::OcppValue ocpp_value = 12.5; // 12.5 kWh
    auto device_data = translator.translateToDevice("MeterValue.Energy.Active.Import.Register", ocpp_value);
    
    ASSERT_TRUE(std::holds_alternative<ocpp_gateway::ocpp::ModbusData>(device_data));
    const auto& modbus_data = std::get<ocpp_gateway::ocpp::ModbusData>(device_data);
    
    ASSERT_EQ(modbus_data.data.size(), 4);
    
    // Convert back the bytes to float to verify
    uint32_t bits = (static_cast<uint32_t>(modbus_data.data[0]) << 24) |
                    (static_cast<uint32_t>(modbus_data.data[1]) << 16) |
                    (static_cast<uint32_t>(modbus_data.data[2]) << 8) |
                    modbus_data.data[3];
    float value;
    std::memcpy(&value, &bits, sizeof(float));
    
    // Should be 125.0 (12.5 / 0.1) due to scaling
    EXPECT_NEAR(value, 125.0, 0.001);
    
    // Test Modbus to OCPP translation for float with scaling
    ocpp_gateway::ocpp::ModbusData test_data;
    // Create a float value of 125.0
    float test_value = 125.0f;
    uint32_t test_bits;
    std::memcpy(&test_bits, &test_value, sizeof(float));
    
    test_data.data = {
        static_cast<uint8_t>(test_bits >> 24),
        static_cast<uint8_t>((test_bits >> 16) & 0xFF),
        static_cast<uint8_t>((test_bits >> 8) & 0xFF),
        static_cast<uint8_t>(test_bits & 0xFF)
    };
    
    ocpp_gateway::ocpp::DeviceData input_data = test_data;
    auto result = translator.translateToOcpp("MeterValue.Energy.Active.Import.Register", input_data);
    
    ASSERT_TRUE(std::holds_alternative<double>(result));
    EXPECT_NEAR(std::get<double>(result), 12.5, 0.001); // 125.0 * 0.1 = 12.5
}

TEST_F(VariableTranslatorTest, TestModbusBooleanTranslation) {
    ocpp_gateway::ocpp::VariableTranslator translator(template_);
    
    // Test Modbus to OCPP translation for boolean
    ocpp_gateway::ocpp::ModbusData test_data;
    test_data.data = {1}; // true
    
    ocpp_gateway::ocpp::DeviceData input_data = test_data;
    auto result = translator.translateToOcpp("Connector.Status", input_data);
    
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    EXPECT_TRUE(std::get<bool>(result));
    
    // Test write to read-only variable
    ocpp_gateway::ocpp::OcppValue ocpp_value = true;
    EXPECT_THROW(translator.translateToDevice("Connector.Status", ocpp_value), ocpp_gateway::ocpp::TranslationError);
}

TEST_F(VariableTranslatorTest, TestEchonetLiteEnumTranslation) {
    ocpp_gateway::ocpp::VariableTranslator translator(template_);
    
    // Test OCPP to ECHONET Lite translation for enum
    ocpp_gateway::ocpp::OcppValue ocpp_value = std::string("Charging");
    auto device_data = translator.translateToDevice("ChargingState", ocpp_value);
    
    ASSERT_TRUE(std::holds_alternative<ocpp_gateway::ocpp::EchonetLiteData>(device_data));
    const auto& el_data = std::get<ocpp_gateway::ocpp::EchonetLiteData>(device_data);
    
    ASSERT_EQ(el_data.data.size(), 1);
    EXPECT_EQ(el_data.data[0], 1); // Value 1 = "Charging"
    
    // Test ECHONET Lite to OCPP translation for enum
    ocpp_gateway::ocpp::EchonetLiteData test_data;
    test_data.data = {2}; // Value 2 = "SuspendedEV"
    
    ocpp_gateway::ocpp::DeviceData input_data = test_data;
    auto result = translator.translateToOcpp("ChargingState", input_data);
    
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    EXPECT_EQ(std::get<std::string>(result), "SuspendedEV");
}

TEST_F(VariableTranslatorTest, TestEchonetLiteIntWithScalingTranslation) {
    ocpp_gateway::ocpp::VariableTranslator translator(template_);
    
    // Test OCPP to ECHONET Lite translation for int with scaling
    ocpp_gateway::ocpp::OcppValue ocpp_value = 16; // 16A
    auto device_data = translator.translateToDevice("Current.Import", ocpp_value);
    
    ASSERT_TRUE(std::holds_alternative<ocpp_gateway::ocpp::EchonetLiteData>(device_data));
    const auto& el_data = std::get<ocpp_gateway::ocpp::EchonetLiteData>(device_data);
    
    ASSERT_EQ(el_data.data.size(), 2);
    
    // Convert back the bytes to int16 to verify
    int16_t value = (static_cast<int16_t>(el_data.data[0]) << 8) | el_data.data[1];
    
    // Should be 160 (16 / 0.1) due to scaling
    EXPECT_EQ(value, 160);
    
    // Test ECHONET Lite to OCPP translation for int with scaling
    ocpp_gateway::ocpp::EchonetLiteData test_data;
    test_data.data = {0, 160}; // Value 160
    
    ocpp_gateway::ocpp::DeviceData input_data = test_data;
    auto result = translator.translateToOcpp("Current.Import", input_data);
    
    ASSERT_TRUE(std::holds_alternative<int>(result));
    EXPECT_EQ(std::get<int>(result), 16); // 160 * 0.1 = 16
}

TEST_F(VariableTranslatorTest, TestInvalidVariableName) {
    ocpp_gateway::ocpp::VariableTranslator translator(template_);
    
    // Test with non-existent variable
    ocpp_gateway::ocpp::OcppValue ocpp_value = 42;
    EXPECT_THROW(translator.translateToDevice("NonExistentVariable", ocpp_value), ocpp_gateway::ocpp::TranslationError);
    
    ocpp_gateway::ocpp::ModbusData test_data;
    test_data.data = {0, 42};
    ocpp_gateway::ocpp::DeviceData input_data = test_data;
    EXPECT_THROW(translator.translateToOcpp("NonExistentVariable", input_data), ocpp_gateway::ocpp::TranslationError);
}

TEST_F(VariableTranslatorTest, TestTypeMismatch) {
    ocpp_gateway::ocpp::VariableTranslator translator(template_);
    
    // Test with wrong data type for variable
    ocpp_gateway::ocpp::OcppValue ocpp_value = std::string("Wrong Type");
    EXPECT_THROW(translator.translateToDevice("MeterValue.Energy.Active.Import.Register", ocpp_value), ocpp_gateway::ocpp::TranslationError);
    
    // Test with wrong device data type
    ocpp_gateway::ocpp::EchonetLiteData el_data;
    el_data.data = {1};
    ocpp_gateway::ocpp::DeviceData input_data = el_data;
    EXPECT_THROW(translator.translateToOcpp("MeterValue.Energy.Active.Import.Register", input_data), ocpp_gateway::ocpp::TranslationError);
}

TEST_F(VariableTranslatorTest, TestInvalidEnumValue) {
    ocpp_gateway::ocpp::VariableTranslator translator(template_);
    
    // Test with invalid enum string
    ocpp_gateway::ocpp::OcppValue ocpp_value = std::string("InvalidEnum");
    EXPECT_THROW(translator.translateToDevice("AvailabilityState", ocpp_value), ocpp_gateway::ocpp::TranslationError);
    
    // Test with invalid enum integer
    ocpp_gateway::ocpp::ModbusData test_data;
    test_data.data = {0, 10}; // Value 10 is not in enum map
    ocpp_gateway::ocpp::DeviceData input_data = test_data;
    EXPECT_THROW(translator.translateToOcpp("AvailabilityState", input_data), ocpp_gateway::ocpp::TranslationError);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}