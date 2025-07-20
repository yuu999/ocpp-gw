#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "ocpp_gateway/device/modbus_rtu_adapter.h"

using namespace ocpp_gateway::device;
using namespace testing;

// Mock ModbusRtuAdapter for testing
class MockModbusRtuAdapter : public ModbusRtuAdapter {
public:
    MOCK_METHOD(bool, initializeSocket, (), (override));
    MOCK_METHOD(std::shared_ptr<ModbusConnection>, createConnection, (const ModbusRtuAddress&), (override));
    MOCK_METHOD(ReadResult, readCoil, (std::shared_ptr<ModbusConnection>, int, const RegisterAddress&), (override));
    MOCK_METHOD(ReadResult, readDiscreteInput, (std::shared_ptr<ModbusConnection>, int, const RegisterAddress&), (override));
    MOCK_METHOD(ReadResult, readInputRegister, (std::shared_ptr<ModbusConnection>, int, const RegisterAddress&), (override));
    MOCK_METHOD(ReadResult, readHoldingRegister, (std::shared_ptr<ModbusConnection>, int, const RegisterAddress&), (override));
    MOCK_METHOD(WriteResult, writeCoil, (std::shared_ptr<ModbusConnection>, int, const RegisterAddress&, const RegisterValue&), (override));
    MOCK_METHOD(WriteResult, writeHoldingRegister, (std::shared_ptr<ModbusConnection>, int, const RegisterAddress&, const RegisterValue&), (override));
};

class ModbusRtuAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter_ = std::make_unique<MockModbusRtuAdapter>();
    }
    
    std::unique_ptr<MockModbusRtuAdapter> adapter_;
};

TEST_F(ModbusRtuAdapterTest, ValidateRegisterAddress) {
    // Valid coil address
    RegisterAddress valid_coil;
    valid_coil.type = RegisterType::COIL;
    valid_coil.address = 100;
    valid_coil.count = 1;
    
    EXPECT_TRUE(adapter_->validateRegisterAddress(valid_coil));
    
    // Valid discrete input address
    RegisterAddress valid_discrete;
    valid_discrete.type = RegisterType::DISCRETE_INPUT;
    valid_discrete.address = 200;
    valid_discrete.count = 1;
    
    EXPECT_TRUE(adapter_->validateRegisterAddress(valid_discrete));
    
    // Valid input register address
    RegisterAddress valid_input;
    valid_input.type = RegisterType::INPUT_REGISTER;
    valid_input.address = 300;
    valid_input.count = 1;
    
    EXPECT_TRUE(adapter_->validateRegisterAddress(valid_input));
    
    // Valid holding register address
    RegisterAddress valid_holding;
    valid_holding.type = RegisterType::HOLDING_REGISTER;
    valid_holding.address = 400;
    valid_holding.count = 1;
    
    EXPECT_TRUE(adapter_->validateRegisterAddress(valid_holding));
    
    // Invalid register type
    RegisterAddress invalid_type;
    invalid_type.type = RegisterType::EPC;
    invalid_type.address = 100;
    invalid_type.count = 1;
    
    EXPECT_FALSE(adapter_->validateRegisterAddress(invalid_type));
    
    // Invalid address (too large)
    RegisterAddress invalid_address;
    invalid_address.type = RegisterType::COIL;
    invalid_address.address = 70000; // Exceeds 16-bit address space
    invalid_address.count = 1;
    
    EXPECT_FALSE(adapter_->validateRegisterAddress(invalid_address));
    
    // Invalid count (zero)
    RegisterAddress invalid_count_zero;
    invalid_count_zero.type = RegisterType::COIL;
    invalid_count_zero.address = 100;
    invalid_count_zero.count = 0;
    
    EXPECT_FALSE(adapter_->validateRegisterAddress(invalid_count_zero));
    
    // Invalid count (too large for bits)
    RegisterAddress invalid_count_bits;
    invalid_count_bits.type = RegisterType::COIL;
    invalid_count_bits.address = 100;
    invalid_count_bits.count = 3000; // Exceeds MODBUS_MAX_READ_BITS
    
    EXPECT_FALSE(adapter_->validateRegisterAddress(invalid_count_bits));
    
    // Invalid count (too large for registers)
    RegisterAddress invalid_count_regs;
    invalid_count_regs.type = RegisterType::HOLDING_REGISTER;
    invalid_count_regs.address = 400;
    invalid_count_regs.count = 200; // Exceeds MODBUS_MAX_READ_REGISTERS
    
    EXPECT_FALSE(adapter_->validateRegisterAddress(invalid_count_regs));
}

TEST_F(ModbusRtuAdapterTest, ValidateDeviceAddress) {
    // Valid address
    DeviceInfo valid_device;
    valid_device.id = "test_device";
    valid_device.protocol = DeviceProtocol::MODBUS_RTU;
    auto address = std::make_shared<ModbusRtuAddress>();
    address->port = "/dev/ttyS0";
    address->baud_rate = 9600;
    address->data_bits = 8;
    address->stop_bits = 1;
    address->parity = "N";
    address->unit_id = 1;
    valid_device.address = address;
    
    EXPECT_TRUE(adapter_->validateDeviceAddress(valid_device));
    
    // Invalid address type
    DeviceInfo invalid_type;
    invalid_type.id = "test_device";
    invalid_type.protocol = DeviceProtocol::MODBUS_RTU;
    auto echonet_address = std::make_shared<EchonetLiteAddress>();
    echonet_address->ip_address = "192.168.1.100";
    invalid_type.address = echonet_address;
    
    EXPECT_FALSE(adapter_->validateDeviceAddress(invalid_type));
    
    // Empty port
    DeviceInfo empty_port;
    empty_port.id = "test_device";
    empty_port.protocol = DeviceProtocol::MODBUS_RTU;
    auto empty_address = std::make_shared<ModbusRtuAddress>();
    empty_address->port = "";
    empty_address->unit_id = 1;
    empty_port.address = empty_address;
    
    EXPECT_FALSE(adapter_->validateDeviceAddress(empty_port));
    
    // Invalid unit ID (too small)
    DeviceInfo invalid_unit_small;
    invalid_unit_small.id = "test_device";
    invalid_unit_small.protocol = DeviceProtocol::MODBUS_RTU;
    auto small_address = std::make_shared<ModbusRtuAddress>();
    small_address->port = "/dev/ttyS0";
    small_address->unit_id = 0; // Valid range is 1-247
    invalid_unit_small.address = small_address;
    
    EXPECT_FALSE(adapter_->validateDeviceAddress(invalid_unit_small));
    
    // Invalid unit ID (too large)
    DeviceInfo invalid_unit_large;
    invalid_unit_large.id = "test_device";
    invalid_unit_large.protocol = DeviceProtocol::MODBUS_RTU;
    auto large_address = std::make_shared<ModbusRtuAddress>();
    large_address->port = "/dev/ttyS0";
    large_address->unit_id = 248; // Valid range is 1-247
    invalid_unit_large.address = large_address;
    
    EXPECT_FALSE(adapter_->validateDeviceAddress(invalid_unit_large));
}

TEST_F(ModbusRtuAdapterTest, ReadRegister) {
    // Set up mock expectations
    EXPECT_CALL(*adapter_, readCoil(_, _, _))
        .WillOnce([](std::shared_ptr<ModbusConnection>, int, const RegisterAddress&) {
            RegisterValue value;
            value.setBool(true);
            return BaseDeviceAdapter::createSuccessReadResult(value);
        });
    
    EXPECT_CALL(*adapter_, readDiscreteInput(_, _, _))
        .WillOnce([](std::shared_ptr<ModbusConnection>, int, const RegisterAddress&) {
            RegisterValue value;
            value.setBool(false);
            return BaseDeviceAdapter::createSuccessReadResult(value);
        });
    
    EXPECT_CALL(*adapter_, readInputRegister(_, _, _))
        .WillOnce([](std::shared_ptr<ModbusConnection>, int, const RegisterAddress&) {
            RegisterValue value;
            value.setUint16(12345);
            return BaseDeviceAdapter::createSuccessReadResult(value);
        });
    
    EXPECT_CALL(*adapter_, readHoldingRegister(_, _, _))
        .WillOnce([](std::shared_ptr<ModbusConnection>, int, const RegisterAddress&) {
            RegisterValue value;
            value.setUint16(54321);
            return BaseDeviceAdapter::createSuccessReadResult(value);
        });
    
    // Add a test device
    DeviceInfo device;
    device.id = "test_device";
    device.protocol = DeviceProtocol::MODBUS_RTU;
    device.online = true;
    auto address = std::make_shared<ModbusRtuAddress>();
    address->port = "/dev/ttyS0";
    address->unit_id = 1;
    device.address = address;
    adapter_->addDevice(device);
    
    // Create register addresses
    RegisterAddress coil_address;
    coil_address.type = RegisterType::COIL;
    coil_address.address = 100;
    coil_address.count = 1;
    
    RegisterAddress discrete_address;
    discrete_address.type = RegisterType::DISCRETE_INPUT;
    discrete_address.address = 200;
    discrete_address.count = 1;
    
    RegisterAddress input_address;
    input_address.type = RegisterType::INPUT_REGISTER;
    input_address.address = 300;
    input_address.count = 1;
    
    RegisterAddress holding_address;
    holding_address.type = RegisterType::HOLDING_REGISTER;
    holding_address.address = 400;
    holding_address.count = 1;
    
    // Test reading different register types
    auto coil_result = adapter_->readRegister("test_device", coil_address);
    EXPECT_TRUE(coil_result.success);
    EXPECT_TRUE(coil_result.value.getBool());
    
    auto discrete_result = adapter_->readRegister("test_device", discrete_address);
    EXPECT_TRUE(discrete_result.success);
    EXPECT_FALSE(discrete_result.value.getBool());
    
    auto input_result = adapter_->readRegister("test_device", input_address);
    EXPECT_TRUE(input_result.success);
    EXPECT_EQ(input_result.value.getUint16(), 12345);
    
    auto holding_result = adapter_->readRegister("test_device", holding_address);
    EXPECT_TRUE(holding_result.success);
    EXPECT_EQ(holding_result.value.getUint16(), 54321);
}

TEST_F(ModbusRtuAdapterTest, WriteRegister) {
    // Set up mock expectations
    EXPECT_CALL(*adapter_, writeCoil(_, _, _, _))
        .WillOnce([](std::shared_ptr<ModbusConnection>, int, const RegisterAddress&, const RegisterValue&) {
            return BaseDeviceAdapter::createSuccessWriteResult();
        });
    
    EXPECT_CALL(*adapter_, writeHoldingRegister(_, _, _, _))
        .WillOnce([](std::shared_ptr<ModbusConnection>, int, const RegisterAddress&, const RegisterValue&) {
            return BaseDeviceAdapter::createSuccessWriteResult();
        });
    
    // Add a test device
    DeviceInfo device;
    device.id = "test_device";
    device.protocol = DeviceProtocol::MODBUS_RTU;
    device.online = true;
    auto address = std::make_shared<ModbusRtuAddress>();
    address->port = "/dev/ttyS0";
    address->unit_id = 1;
    device.address = address;
    adapter_->addDevice(device);
    
    // Create register addresses
    RegisterAddress coil_address;
    coil_address.type = RegisterType::COIL;
    coil_address.address = 100;
    coil_address.count = 1;
    
    RegisterAddress holding_address;
    holding_address.type = RegisterType::HOLDING_REGISTER;
    holding_address.address = 400;
    holding_address.count = 1;
    
    RegisterAddress input_address;
    input_address.type = RegisterType::INPUT_REGISTER;
    input_address.address = 300;
    input_address.count = 1;
    
    // Create register values
    RegisterValue coil_value;
    coil_value.setBool(true);
    
    RegisterValue holding_value;
    holding_value.setUint16(12345);
    
    // Test writing to writable registers
    auto coil_result = adapter_->writeRegister("test_device", coil_address, coil_value);
    EXPECT_TRUE(coil_result.success);
    
    auto holding_result = adapter_->writeRegister("test_device", holding_address, holding_value);
    EXPECT_TRUE(holding_result.success);
    
    // Test writing to read-only register
    auto input_result = adapter_->writeRegister("test_device", input_address, holding_value);
    EXPECT_FALSE(input_result.success);
    EXPECT_EQ(input_result.error_message, "Cannot write to read-only register type");
}

TEST_F(ModbusRtuAdapterTest, ErrorHandling) {
    // Add a test device
    DeviceInfo device;
    device.id = "test_device";
    device.protocol = DeviceProtocol::MODBUS_RTU;
    device.online = true;
    auto address = std::make_shared<ModbusRtuAddress>();
    address->port = "/dev/ttyS0";
    address->unit_id = 1;
    device.address = address;
    adapter_->addDevice(device);
    
    // Create register address
    RegisterAddress reg_address;
    reg_address.type = RegisterType::HOLDING_REGISTER;
    reg_address.address = 400;
    reg_address.count = 1;
    
    // Test device not found
    auto not_found_result = adapter_->readRegister("non_existent", reg_address);
    EXPECT_FALSE(not_found_result.success);
    EXPECT_EQ(not_found_result.error_message, "Device not found");
    
    // Test invalid register type
    RegisterAddress invalid_address;
    invalid_address.type = RegisterType::EPC;
    invalid_address.address = 100;
    invalid_address.count = 1;
    
    auto invalid_type_result = adapter_->readRegister("test_device", invalid_address);
    EXPECT_FALSE(invalid_type_result.success);
    EXPECT_EQ(invalid_type_result.error_message, "Invalid register address for Modbus RTU");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}