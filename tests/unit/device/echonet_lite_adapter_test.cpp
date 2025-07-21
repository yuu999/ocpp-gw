#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "ocpp_gateway/device/echonet_lite_adapter.h"

using namespace ocpp_gateway::device;
using namespace testing;

class EchonetLiteFrameTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
};

TEST_F(EchonetLiteFrameTest, SerializeDeserialize) {
    // Create a test frame
    EchonetLiteFrame frame;
    frame.tid = 0x1234;
    frame.seoj_class_group_code = 0x05;
    frame.seoj_class_code = 0xFF;
    frame.seoj_instance_code = 0x01;
    frame.deoj_class_group_code = 0x02;
    frame.deoj_class_code = EOJ_EV_CHARGER_CLASS;
    frame.deoj_instance_code = 0x01;
    frame.esv = 0x62; // GET request
    
    // Add a property
    EchonetLiteFrame::Property prop;
    prop.epc = 0x80; // Operation status
    prop.pdc = 0;    // No data for GET request
    frame.properties.push_back(prop);
    
    // Serialize
    std::vector<uint8_t> data = frame.serialize();
    
    // Expected data
    std::vector<uint8_t> expected = {
        0x10, 0x81,             // EHD
        0x12, 0x34,             // TID
        0x05, 0xFF, 0x01,       // SEOJ
        0x02, 0xA1, 0x01,       // DEOJ
        0x62,                   // ESV
        0x01,                   // OPC
        0x80, 0x00              // EPC, PDC
    };
    
    ASSERT_EQ(data, expected);
    
    // Deserialize
    auto result = EchonetLiteFrame::deserialize(data);
    ASSERT_TRUE(result.has_value());
    
    // Verify deserialized frame
    const auto& deserialized = result.value();
    EXPECT_EQ(deserialized.tid, 0x1234);
    EXPECT_EQ(deserialized.seoj_class_group_code, 0x05);
    EXPECT_EQ(deserialized.seoj_class_code, 0xFF);
    EXPECT_EQ(deserialized.seoj_instance_code, 0x01);
    EXPECT_EQ(deserialized.deoj_class_group_code, 0x02);
    EXPECT_EQ(deserialized.deoj_class_code, EOJ_EV_CHARGER_CLASS);
    EXPECT_EQ(deserialized.deoj_instance_code, 0x01);
    EXPECT_EQ(deserialized.esv, 0x62);
    EXPECT_EQ(deserialized.opc, 1);
    ASSERT_EQ(deserialized.properties.size(), 1);
    EXPECT_EQ(deserialized.properties[0].epc, 0x80);
    EXPECT_EQ(deserialized.properties[0].pdc, 0);
    EXPECT_TRUE(deserialized.properties[0].edt.empty());
}

TEST_F(EchonetLiteFrameTest, CreateGetPropertyFrame) {
    // Create a GET property frame
    std::vector<uint8_t> properties = {0x80, 0x88, 0x8A};
    EchonetLiteFrame frame = EchonetLiteFrame::createGetPropertyFrame(0x02, EOJ_EV_CHARGER_CLASS, 0x01, properties);
    
    // Verify frame
    EXPECT_EQ(frame.deoj_class_group_code, 0x02);
    EXPECT_EQ(frame.deoj_class_code, EOJ_EV_CHARGER_CLASS);
    EXPECT_EQ(frame.deoj_instance_code, 0x01);
    EXPECT_EQ(frame.esv, 0x62); // GET request
    ASSERT_EQ(frame.properties.size(), 3);
    EXPECT_EQ(frame.properties[0].epc, 0x80);
    EXPECT_EQ(frame.properties[0].pdc, 0);
    EXPECT_EQ(frame.properties[1].epc, 0x88);
    EXPECT_EQ(frame.properties[1].pdc, 0);
    EXPECT_EQ(frame.properties[2].epc, 0x8A);
    EXPECT_EQ(frame.properties[2].pdc, 0);
}

TEST_F(EchonetLiteFrameTest, CreateSetPropertyFrame) {
    // Create a SET property frame
    std::vector<uint8_t> edt = {0x30};
    EchonetLiteFrame frame = EchonetLiteFrame::createSetPropertyFrame(0x02, EOJ_EV_CHARGER_CLASS, 0x01, 0x80, edt);
    
    // Verify frame
    EXPECT_EQ(frame.deoj_class_group_code, 0x02);
    EXPECT_EQ(frame.deoj_class_code, EOJ_EV_CHARGER_CLASS);
    EXPECT_EQ(frame.deoj_instance_code, 0x01);
    EXPECT_EQ(frame.esv, 0x61); // SET request
    ASSERT_EQ(frame.properties.size(), 1);
    EXPECT_EQ(frame.properties[0].epc, 0x80);
    EXPECT_EQ(frame.properties[0].pdc, 1);
    ASSERT_EQ(frame.properties[0].edt.size(), 1);
    EXPECT_EQ(frame.properties[0].edt[0], 0x30);
}

TEST_F(EchonetLiteFrameTest, CreateDiscoveryFrame) {
    // Create a discovery frame
    EchonetLiteFrame frame = EchonetLiteFrame::createDiscoveryFrame();
    
    // Verify frame
    EXPECT_EQ(frame.deoj_class_group_code, 0x0E);
    EXPECT_EQ(frame.deoj_class_code, 0xF0);
    EXPECT_EQ(frame.deoj_instance_code, 0x01);
    EXPECT_EQ(frame.esv, 0x62); // GET request
    ASSERT_EQ(frame.properties.size(), 1);
    EXPECT_EQ(frame.properties[0].epc, 0xD6); // Self node instance list S
    EXPECT_EQ(frame.properties[0].pdc, 0);
}

// Mock EchonetLiteAdapter for testing
class MockEchonetLiteAdapter : public EchonetLiteAdapter {
public:
    MOCK_METHOD(bool, initializeSocket, (), (override));
    MOCK_METHOD(bool, initializeMulticastSocket, (), (override));
    MOCK_METHOD(bool, sendFrame, (const std::string&, const EchonetLiteFrame&), (override));
    MOCK_METHOD(bool, sendMulticastFrame, (const EchonetLiteFrame&), (override));
    MOCK_METHOD(std::optional<EchonetLiteResponse>, sendRequestWithResponse, 
               (const std::string&, const EchonetLiteFrame&, std::chrono::milliseconds), (override));
};

class EchonetLiteAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter_ = std::make_unique<MockEchonetLiteAdapter>();
    }
    
    std::unique_ptr<MockEchonetLiteAdapter> adapter_;
};

TEST_F(EchonetLiteAdapterTest, ValidateRegisterAddress) {
    // Valid address
    RegisterAddress valid_address;
    valid_address.type = RegisterType::EPC;
    valid_address.eoj_class_group_code = 0x02;
    valid_address.eoj_class_code = EOJ_EV_CHARGER_CLASS;
    valid_address.eoj_instance_code = 0x01;
    valid_address.epc = 0x80;
    
    EXPECT_TRUE(adapter_->validateRegisterAddress(valid_address));
    
    // Invalid type
    RegisterAddress invalid_type;
    invalid_type.type = RegisterType::COIL;
    invalid_type.eoj_class_group_code = 0x02;
    invalid_type.eoj_class_code = EOJ_EV_CHARGER_CLASS;
    invalid_type.eoj_instance_code = 0x01;
    invalid_type.epc = 0x80;
    
    EXPECT_FALSE(adapter_->validateRegisterAddress(invalid_type));
    
    // Invalid EOJ
    RegisterAddress invalid_eoj;
    invalid_eoj.type = RegisterType::EPC;
    invalid_eoj.eoj_class_group_code = 0x00;
    invalid_eoj.eoj_class_code = 0x00;
    invalid_eoj.eoj_instance_code = 0x01;
    invalid_eoj.epc = 0x80;
    
    EXPECT_FALSE(adapter_->validateRegisterAddress(invalid_eoj));
}

TEST_F(EchonetLiteAdapterTest, ValidateDeviceAddress) {
    // Valid address
    DeviceInfo valid_device;
    valid_device.id = "test_device";
    valid_device.protocol = DeviceProtocol::ECHONET_LITE;
    auto address = std::make_shared<EchonetLiteAddress>();
    address->ip_address = "192.168.1.100";
    address->port = 3610;
    valid_device.address = address;
    
    EXPECT_TRUE(adapter_->validateDeviceAddress(valid_device));
    
    // Invalid address type
    DeviceInfo invalid_type;
    invalid_type.id = "test_device";
    invalid_type.protocol = DeviceProtocol::ECHONET_LITE;
    auto modbus_address = std::make_shared<ModbusTcpAddress>();
    modbus_address->ip_address = "192.168.1.100";
    invalid_type.address = modbus_address;
    
    EXPECT_FALSE(adapter_->validateDeviceAddress(invalid_type));
    
    // Empty IP address
    DeviceInfo empty_ip;
    empty_ip.id = "test_device";
    empty_ip.protocol = DeviceProtocol::ECHONET_LITE;
    auto empty_address = std::make_shared<EchonetLiteAddress>();
    empty_address->ip_address = "";
    empty_ip.address = empty_address;
    
    EXPECT_FALSE(adapter_->validateDeviceAddress(empty_ip));
}

TEST_F(EchonetLiteAdapterTest, ReadRegister) {
    // Set up mock
    EXPECT_CALL(*adapter_, sendRequestWithResponse(_, _, _))
        .WillOnce(Return(std::nullopt))
        .WillOnce([](const std::string&, const EchonetLiteFrame& frame, std::chrono::milliseconds) {
            // Create a response
            EchonetLiteResponse response;
            response.tid = frame.tid;
            response.success = true;
            
            // Set up response frame
            response.frame.tid = frame.tid;
            response.frame.seoj_class_group_code = frame.deoj_class_group_code;
            response.frame.seoj_class_code = frame.deoj_class_code;
            response.frame.seoj_instance_code = frame.deoj_instance_code;
            response.frame.deoj_class_group_code = frame.seoj_class_group_code;
            response.frame.deoj_class_code = frame.seoj_class_code;
            response.frame.deoj_instance_code = frame.seoj_instance_code;
            response.frame.esv = 0x72; // GET response
            
            // Add property
            EchonetLiteFrame::Property prop;
            prop.epc = 0x80; // Operation status
            prop.pdc = 1;
            prop.edt = {0x30}; // ON
            response.frame.properties.push_back(prop);
            
            return std::optional<EchonetLiteResponse>(response);
        });
    
    // Add a test device
    DeviceInfo device;
    device.id = "test_device";
    device.protocol = DeviceProtocol::ECHONET_LITE;
    device.online = true;
    auto address = std::make_shared<EchonetLiteAddress>();
    address->ip_address = "192.168.1.100";
    device.address = address;
    adapter_->addDevice(device);
    
    // Create register address
    RegisterAddress reg_address;
    reg_address.type = RegisterType::EPC;
    reg_address.eoj_class_group_code = 0x02;
    reg_address.eoj_class_code = EOJ_EV_CHARGER_CLASS;
    reg_address.eoj_instance_code = 0x01;
    reg_address.epc = 0x80;
    
    // Test no response
    auto result1 = adapter_->readRegister("test_device", reg_address);
    EXPECT_FALSE(result1.success);
    EXPECT_EQ(result1.error_message, "No response from device");
    
    // Test successful read
    auto result2 = adapter_->readRegister("test_device", reg_address);
    EXPECT_TRUE(result2.success);
    ASSERT_EQ(result2.value.data.size(), 1);
    EXPECT_EQ(result2.value.data[0], 0x30);
}

TEST_F(EchonetLiteAdapterTest, WriteRegister) {
    // Set up mock
    EXPECT_CALL(*adapter_, sendRequestWithResponse(_, _, _))
        .WillOnce([](const std::string&, const EchonetLiteFrame& frame, std::chrono::milliseconds) {
            // Create a response
            EchonetLiteResponse response;
            response.tid = frame.tid;
            response.success = true;
            
            // Set up response frame
            response.frame.tid = frame.tid;
            response.frame.seoj_class_group_code = frame.deoj_class_group_code;
            response.frame.seoj_class_code = frame.deoj_class_code;
            response.frame.seoj_instance_code = frame.deoj_instance_code;
            response.frame.deoj_class_group_code = frame.seoj_class_group_code;
            response.frame.deoj_class_code = frame.seoj_class_code;
            response.frame.deoj_instance_code = frame.seoj_instance_code;
            response.frame.esv = 0x71; // SET response
            
            // Add property
            EchonetLiteFrame::Property prop;
            prop.epc = 0x80; // Operation status
            prop.pdc = 0;
            response.frame.properties.push_back(prop);
            
            return std::optional<EchonetLiteResponse>(response);
        });
    
    // Add a test device
    DeviceInfo device;
    device.id = "test_device";
    device.protocol = DeviceProtocol::ECHONET_LITE;
    device.online = true;
    auto address = std::make_shared<EchonetLiteAddress>();
    address->ip_address = "192.168.1.100";
    device.address = address;
    adapter_->addDevice(device);
    
    // Create register address
    RegisterAddress reg_address;
    reg_address.type = RegisterType::EPC;
    reg_address.eoj_class_group_code = 0x02;
    reg_address.eoj_class_code = EOJ_EV_CHARGER_CLASS;
    reg_address.eoj_instance_code = 0x01;
    reg_address.epc = 0x80;
    
    // Create register value
    RegisterValue value;
    value.type = DataType::UINT8;
    value.data = {0x30}; // ON
    
    // Test successful write
    auto result = adapter_->writeRegister("test_device", reg_address, value);
    EXPECT_TRUE(result.success);
}

TEST_F(EchonetLiteAdapterTest, ReadMultipleRegisters) {
    // Set up mock
    EXPECT_CALL(*adapter_, sendRequestWithResponse(_, _, _))
        .WillOnce([](const std::string&, const EchonetLiteFrame& frame, std::chrono::milliseconds) {
            // Create a response
            EchonetLiteResponse response;
            response.tid = frame.tid;
            response.success = true;
            
            // Set up response frame
            response.frame.tid = frame.tid;
            response.frame.seoj_class_group_code = frame.deoj_class_group_code;
            response.frame.seoj_class_code = frame.deoj_class_code;
            response.frame.seoj_instance_code = frame.deoj_instance_code;
            response.frame.deoj_class_group_code = frame.seoj_class_group_code;
            response.frame.deoj_class_code = frame.seoj_class_code;
            response.frame.deoj_instance_code = frame.seoj_instance_code;
            response.frame.esv = 0x72; // GET response
            
            // Add properties
            EchonetLiteFrame::Property prop1;
            prop1.epc = 0x80; // Operation status
            prop1.pdc = 1;
            prop1.edt = {0x30}; // ON
            response.frame.properties.push_back(prop1);
            
            EchonetLiteFrame::Property prop2;
            prop2.epc = 0x88; // Fault status
            prop2.pdc = 1;
            prop2.edt = {0x42}; // Fault
            response.frame.properties.push_back(prop2);
            
            return std::optional<EchonetLiteResponse>(response);
        });
    
    // Add a test device
    DeviceInfo device;
    device.id = "test_device";
    device.protocol = DeviceProtocol::ECHONET_LITE;
    device.online = true;
    auto address = std::make_shared<EchonetLiteAddress>();
    address->ip_address = "192.168.1.100";
    device.address = address;
    adapter_->addDevice(device);
    
    // Create register addresses
    RegisterAddress reg_address1;
    reg_address1.type = RegisterType::EPC;
    reg_address1.eoj_class_group_code = 0x02;
    reg_address1.eoj_class_code = EOJ_EV_CHARGER_CLASS;
    reg_address1.eoj_instance_code = 0x01;
    reg_address1.epc = 0x80;
    
    RegisterAddress reg_address2;
    reg_address2.type = RegisterType::EPC;
    reg_address2.eoj_class_group_code = 0x02;
    reg_address2.eoj_class_code = EOJ_EV_CHARGER_CLASS;
    reg_address2.eoj_instance_code = 0x01;
    reg_address2.epc = 0x88;
    
    // Test reading multiple registers
    std::vector<RegisterAddress> addresses = {reg_address1, reg_address2};
    auto results = adapter_->readMultipleRegisters("test_device", addresses);
    
    ASSERT_EQ(results.size(), 2);
    
    // Check first result
    ASSERT_TRUE(results.find(reg_address1) != results.end());
    EXPECT_TRUE(results[reg_address1].success);
    ASSERT_EQ(results[reg_address1].value.data.size(), 1);
    EXPECT_EQ(results[reg_address1].value.data[0], 0x30);
    
    // Check second result
    ASSERT_TRUE(results.find(reg_address2) != results.end());
    EXPECT_TRUE(results[reg_address2].success);
    ASSERT_EQ(results[reg_address2].value.data.size(), 1);
    EXPECT_EQ(results[reg_address2].value.data[0], 0x42);
}