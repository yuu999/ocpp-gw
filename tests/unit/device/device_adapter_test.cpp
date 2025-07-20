#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <thread>
#include <chrono>

#include "ocpp_gateway/device/device_adapter.h"

using namespace ocpp_gateway::device;
using namespace testing;

// Mock implementation of DeviceAdapter for testing
class MockDeviceAdapter : public BaseDeviceAdapter {
public:
    MockDeviceAdapter() : BaseDeviceAdapter(DeviceProtocol::ECHONET_LITE) {}
    
    MOCK_METHOD(bool, startDiscovery, (DeviceDiscoveryCallback, std::chrono::milliseconds), (override));
    MOCK_METHOD(void, stopDiscovery, (), (override));
    MOCK_METHOD(bool, isDiscoveryInProgress, (), (const, override));
    MOCK_METHOD(ReadResult, readRegister, (const std::string&, const RegisterAddress&), (override));
    MOCK_METHOD(WriteResult, writeRegister, (const std::string&, const RegisterAddress&, const RegisterValue&), (override));
};

class DeviceAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter = std::make_unique<MockDeviceAdapter>();
        adapter->initialize();
        adapter->start();
    }
    
    void TearDown() override {
        adapter->stop();
        adapter.reset();
    }
    
    std::unique_ptr<MockDeviceAdapter> adapter;
};

TEST_F(DeviceAdapterTest, ProtocolConversion) {
    EXPECT_EQ(protocolFromString("echonet_lite"), DeviceProtocol::ECHONET_LITE);
    EXPECT_EQ(protocolFromString("ECHONET_LITE"), DeviceProtocol::ECHONET_LITE);
    EXPECT_EQ(protocolFromString("echonetlite"), DeviceProtocol::ECHONET_LITE);
    EXPECT_EQ(protocolFromString("el"), DeviceProtocol::ECHONET_LITE);
    
    EXPECT_EQ(protocolFromString("modbus_rtu"), DeviceProtocol::MODBUS_RTU);
    EXPECT_EQ(protocolFromString("MODBUS_RTU"), DeviceProtocol::MODBUS_RTU);
    EXPECT_EQ(protocolFromString("modbusrtu"), DeviceProtocol::MODBUS_RTU);
    
    EXPECT_EQ(protocolFromString("modbus_tcp"), DeviceProtocol::MODBUS_TCP);
    EXPECT_EQ(protocolFromString("MODBUS_TCP"), DeviceProtocol::MODBUS_TCP);
    EXPECT_EQ(protocolFromString("modbustcp"), DeviceProtocol::MODBUS_TCP);
    
    EXPECT_EQ(protocolFromString("unknown"), DeviceProtocol::UNKNOWN);
    EXPECT_EQ(protocolFromString(""), DeviceProtocol::UNKNOWN);
    
    EXPECT_EQ(protocolToString(DeviceProtocol::ECHONET_LITE), "ECHONET_LITE");
    EXPECT_EQ(protocolToString(DeviceProtocol::MODBUS_RTU), "MODBUS_RTU");
    EXPECT_EQ(protocolToString(DeviceProtocol::MODBUS_TCP), "MODBUS_TCP");
    EXPECT_EQ(protocolToString(DeviceProtocol::UNKNOWN), "UNKNOWN");
}

TEST_F(DeviceAdapterTest, RegisterValueBool) {
    RegisterValue value;
    value.setBool(true);
    
    EXPECT_EQ(value.type, DataType::BOOL);
    EXPECT_EQ(value.data.size(), 1);
    EXPECT_EQ(value.data[0], 1);
    EXPECT_TRUE(value.getBool());
    
    value.setBool(false);
    EXPECT_EQ(value.data[0], 0);
    EXPECT_FALSE(value.getBool());
}

TEST_F(DeviceAdapterTest, RegisterValueInteger) {
    RegisterValue value;
    
    // Test uint8
    value.setUint8(42);
    EXPECT_EQ(value.type, DataType::UINT8);
    EXPECT_EQ(value.data.size(), 1);
    EXPECT_EQ(value.getUint8(), 42);
    
    // Test int8
    value.setInt8(-42);
    EXPECT_EQ(value.type, DataType::INT8);
    EXPECT_EQ(value.data.size(), 1);
    EXPECT_EQ(value.getInt8(), -42);
    
    // Test uint16
    value.setUint16(12345);
    EXPECT_EQ(value.type, DataType::UINT16);
    EXPECT_EQ(value.data.size(), 2);
    EXPECT_EQ(value.getUint16(), 12345);
    
    // Test int16
    value.setInt16(-12345);
    EXPECT_EQ(value.type, DataType::INT16);
    EXPECT_EQ(value.data.size(), 2);
    EXPECT_EQ(value.getInt16(), -12345);
    
    // Test uint32
    value.setUint32(1234567890);
    EXPECT_EQ(value.type, DataType::UINT32);
    EXPECT_EQ(value.data.size(), 4);
    EXPECT_EQ(value.getUint32(), 1234567890);
    
    // Test int32
    value.setInt32(-1234567890);
    EXPECT_EQ(value.type, DataType::INT32);
    EXPECT_EQ(value.data.size(), 4);
    EXPECT_EQ(value.getInt32(), -1234567890);
    
    // Test uint64
    value.setUint64(1234567890123456789ULL);
    EXPECT_EQ(value.type, DataType::UINT64);
    EXPECT_EQ(value.data.size(), 8);
    EXPECT_EQ(value.getUint64(), 1234567890123456789ULL);
    
    // Test int64
    value.setInt64(-1234567890123456789LL);
    EXPECT_EQ(value.type, DataType::INT64);
    EXPECT_EQ(value.data.size(), 8);
    EXPECT_EQ(value.getInt64(), -1234567890123456789LL);
}

TEST_F(DeviceAdapterTest, RegisterValueFloat) {
    RegisterValue value;
    
    // Test float32
    float f32 = 3.14159f;
    value.setFloat32(f32);
    EXPECT_EQ(value.type, DataType::FLOAT32);
    EXPECT_EQ(value.data.size(), 4);
    EXPECT_FLOAT_EQ(value.getFloat32(), f32);
    
    // Test float64
    double f64 = 3.14159265358979323846;
    value.setFloat64(f64);
    EXPECT_EQ(value.type, DataType::FLOAT64);
    EXPECT_EQ(value.data.size(), 8);
    EXPECT_DOUBLE_EQ(value.getFloat64(), f64);
}

TEST_F(DeviceAdapterTest, RegisterValueString) {
    RegisterValue value;
    
    std::string test_str = "Hello, World!";
    value.setString(test_str);
    EXPECT_EQ(value.type, DataType::STRING);
    EXPECT_EQ(value.data.size(), test_str.size());
    EXPECT_EQ(value.getString(), test_str);
}

TEST_F(DeviceAdapterTest, RegisterValueBinary) {
    RegisterValue value;
    
    std::vector<uint8_t> binary_data = {0x01, 0x02, 0x03, 0x04, 0x05};
    value.setBinary(binary_data);
    EXPECT_EQ(value.type, DataType::BINARY);
    EXPECT_EQ(value.data, binary_data);
}

TEST_F(DeviceAdapterTest, DeviceManagement) {
    // Create test device
    DeviceInfo device;
    device.id = "test_device";
    device.name = "Test Device";
    device.model = "Test Model";
    device.manufacturer = "Test Manufacturer";
    device.protocol = DeviceProtocol::ECHONET_LITE;
    device.address = std::make_shared<EchonetLiteAddress>();
    
    // Add device
    EXPECT_TRUE(adapter->addDevice(device));
    
    // Try to add the same device again
    EXPECT_FALSE(adapter->addDevice(device));
    
    // Get device info
    auto device_info = adapter->getDeviceInfo("test_device");
    EXPECT_TRUE(device_info.has_value());
    EXPECT_EQ(device_info->id, "test_device");
    EXPECT_EQ(device_info->name, "Test Device");
    
    // Get non-existent device
    EXPECT_FALSE(adapter->getDeviceInfo("non_existent").has_value());
    
    // Get all devices
    auto devices = adapter->getAllDevices();
    EXPECT_EQ(devices.size(), 1);
    EXPECT_EQ(devices[0].id, "test_device");
    
    // Check device status
    EXPECT_FALSE(adapter->isDeviceOnline("test_device"));
    
    // Set device status callback
    bool callback_called = false;
    std::string callback_device_id;
    bool callback_status = false;
    
    EXPECT_TRUE(adapter->setDeviceStatusCallback("test_device", 
        [&](const std::string& device_id, bool online) {
            callback_called = true;
            callback_device_id = device_id;
            callback_status = online;
        }
    ));
    
    // Update device status
    adapter->updateDeviceStatus("test_device", true);
    
    // Check if callback was called
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(callback_device_id, "test_device");
    EXPECT_TRUE(callback_status);
    
    // Check device status again
    EXPECT_TRUE(adapter->isDeviceOnline("test_device"));
    
    // Remove device
    EXPECT_TRUE(adapter->removeDevice("test_device"));
    
    // Try to remove non-existent device
    EXPECT_FALSE(adapter->removeDevice("test_device"));
    
    // Check that device list is empty
    EXPECT_EQ(adapter->getAllDevices().size(), 0);
}

TEST_F(DeviceAdapterTest, AsyncOperations) {
    // Set up mock expectations
    RegisterAddress address;
    address.type = RegisterType::EPC;
    address.epc = 0x80;
    
    RegisterValue value;
    value.setBool(true);
    
    ReadResult read_result = BaseDeviceAdapter::createSuccessReadResult(value);
    WriteResult write_result = BaseDeviceAdapter::createSuccessWriteResult();
    
    EXPECT_CALL(*adapter, readRegister("test_device", address))
        .WillOnce(Return(read_result));
    
    EXPECT_CALL(*adapter, writeRegister("test_device", address, value))
        .WillOnce(Return(write_result));
    
    // Test async read
    auto read_future = adapter->readRegisterAsync("test_device", address);
    auto async_read_result = read_future.get();
    
    EXPECT_TRUE(async_read_result.success);
    EXPECT_EQ(async_read_result.value.getBool(), true);
    
    // Test async write
    auto write_future = adapter->writeRegisterAsync("test_device", address, value);
    auto async_write_result = write_future.get();
    
    EXPECT_TRUE(async_write_result.success);
}

TEST_F(DeviceAdapterTest, BatchOperations) {
    // Set up mock expectations
    RegisterAddress address1;
    address1.type = RegisterType::EPC;
    address1.epc = 0x80;
    
    RegisterAddress address2;
    address2.type = RegisterType::EPC;
    address2.epc = 0x81;
    
    RegisterValue value1;
    value1.setBool(true);
    
    RegisterValue value2;
    value2.setUint8(42);
    
    ReadResult read_result1 = BaseDeviceAdapter::createSuccessReadResult(value1);
    ReadResult read_result2 = BaseDeviceAdapter::createSuccessReadResult(value2);
    
    WriteResult write_result1 = BaseDeviceAdapter::createSuccessWriteResult();
    WriteResult write_result2 = BaseDeviceAdapter::createSuccessWriteResult();
    
    EXPECT_CALL(*adapter, readRegister("test_device", address1))
        .WillOnce(Return(read_result1));
    
    EXPECT_CALL(*adapter, readRegister("test_device", address2))
        .WillOnce(Return(read_result2));
    
    EXPECT_CALL(*adapter, writeRegister("test_device", address1, value1))
        .WillOnce(Return(write_result1));
    
    EXPECT_CALL(*adapter, writeRegister("test_device", address2, value2))
        .WillOnce(Return(write_result2));
    
    // Test batch read
    std::vector<RegisterAddress> read_addresses = {address1, address2};
    auto batch_read_results = adapter->readMultipleRegisters("test_device", read_addresses);
    
    EXPECT_EQ(batch_read_results.size(), 2);
    EXPECT_TRUE(batch_read_results[address1].success);
    EXPECT_TRUE(batch_read_results[address2].success);
    EXPECT_EQ(batch_read_results[address1].value.getBool(), true);
    EXPECT_EQ(batch_read_results[address2].value.getUint8(), 42);
    
    // Test batch write
    std::map<RegisterAddress, RegisterValue> write_values = {
        {address1, value1},
        {address2, value2}
    };
    
    auto batch_write_results = adapter->writeMultipleRegisters("test_device", write_values);
    
    EXPECT_EQ(batch_write_results.size(), 2);
    EXPECT_TRUE(batch_write_results[address1].success);
    EXPECT_TRUE(batch_write_results[address2].success);
}

TEST_F(DeviceAdapterTest, ErrorHandling) {
    // Test error read result
    ReadResult error_read = BaseDeviceAdapter::createErrorReadResult("Test error", 123);
    EXPECT_FALSE(error_read.success);
    EXPECT_EQ(error_read.error_message, "Test error");
    EXPECT_EQ(error_read.error_code, 123);
    
    // Test error write result
    WriteResult error_write = BaseDeviceAdapter::createErrorWriteResult("Test error", 456);
    EXPECT_FALSE(error_write.success);
    EXPECT_EQ(error_write.error_message, "Test error");
    EXPECT_EQ(error_write.error_code, 456);
    
    // Test success read result
    RegisterValue value;
    value.setUint16(12345);
    
    ReadResult success_read = BaseDeviceAdapter::createSuccessReadResult(value);
    EXPECT_TRUE(success_read.success);
    EXPECT_EQ(success_read.value.getUint16(), 12345);
    
    // Test success write result
    WriteResult success_write = BaseDeviceAdapter::createSuccessWriteResult();
    EXPECT_TRUE(success_write.success);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}