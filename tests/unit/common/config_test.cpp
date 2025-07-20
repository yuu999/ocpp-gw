#include <gtest/gtest.h>
#include "ocpp_gateway/common/system_config.h"
#include "ocpp_gateway/common/csms_config.h"
#include "ocpp_gateway/common/device_config.h"
#include "ocpp_gateway/common/config_manager.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace ocpp_gateway {
namespace config {
namespace test {

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        temp_dir_ = fs::temp_directory_path() / "ocpp_gateway_test";
        fs::create_directories(temp_dir_);
        fs::create_directories(temp_dir_ / "devices");
    }

    void TearDown() override {
        // Remove temporary directory
        fs::remove_all(temp_dir_);
    }

    // Helper method to create a test YAML file
    void createYamlFile(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        file << content;
        file.close();
    }

    fs::path temp_dir_;
};

TEST_F(ConfigTest, SystemConfigLoadFromYaml) {
    // Create test YAML file
    std::string yaml_content = R"(
system:
  log_level: DEBUG
  log_rotation:
    max_size_mb: 20
    max_files: 10
  metrics:
    prometheus_port: 8080
  security:
    tls_cert_path: "/path/to/cert.pem"
    tls_key_path: "/path/to/key.pem"
    ca_cert_path: "/path/to/ca.pem"
    client_cert_required: true
)";
    
    std::string yaml_path = (temp_dir_ / "system.yaml").string();
    createYamlFile(yaml_path, yaml_content);
    
    // Load configuration from YAML file
    SystemConfig config(yaml_path);
    
    // Verify loaded values
    EXPECT_EQ(config.getLogLevel(), LogLevel::DEBUG);
    EXPECT_EQ(config.getLogRotation().max_size_mb, 20);
    EXPECT_EQ(config.getLogRotation().max_files, 10);
    EXPECT_EQ(config.getMetrics().prometheus_port, 8080);
    EXPECT_EQ(config.getSecurity().tls_cert_path, "/path/to/cert.pem");
    EXPECT_EQ(config.getSecurity().tls_key_path, "/path/to/key.pem");
    EXPECT_EQ(config.getSecurity().ca_cert_path, "/path/to/ca.pem");
    EXPECT_EQ(config.getSecurity().client_cert_required, true);
}

TEST_F(ConfigTest, SystemConfigValidation) {
    SystemConfig config;
    
    // Valid configuration
    EXPECT_NO_THROW(config.validate());
    
    // Invalid log rotation
    LogRotationConfig log_rotation;
    log_rotation.max_size_mb = -1;
    config.setLogRotation(log_rotation);
    EXPECT_THROW(config.validate(), ConfigValidationError);
    
    // Reset to valid
    log_rotation.max_size_mb = 10;
    config.setLogRotation(log_rotation);
    EXPECT_NO_THROW(config.validate());
    
    // Invalid metrics
    MetricsConfig metrics;
    metrics.prometheus_port = 70000;
    config.setMetrics(metrics);
    EXPECT_THROW(config.validate(), ConfigValidationError);
}

TEST_F(ConfigTest, CsmsConfigLoadFromYaml) {
    // Create test YAML file
    std::string yaml_content = R"(
csms:
  url: "wss://test-csms.example.com/ocpp"
  reconnect_interval_sec: 60
  max_reconnect_attempts: 5
  heartbeat_interval_sec: 600
)";
    
    std::string yaml_path = (temp_dir_ / "csms.yaml").string();
    createYamlFile(yaml_path, yaml_content);
    
    // Load configuration from YAML file
    CsmsConfig config(yaml_path);
    
    // Verify loaded values
    EXPECT_EQ(config.getUrl(), "wss://test-csms.example.com/ocpp");
    EXPECT_EQ(config.getReconnectInterval(), 60);
    EXPECT_EQ(config.getMaxReconnectAttempts(), 5);
    EXPECT_EQ(config.getHeartbeatInterval(), 600);
}

TEST_F(ConfigTest, CsmsConfigValidation) {
    CsmsConfig config;
    
    // Valid configuration
    EXPECT_NO_THROW(config.validate());
    
    // Invalid URL
    config.setUrl("");
    EXPECT_THROW(config.validate(), ConfigValidationError);
    
    // Reset to valid
    config.setUrl("wss://csms.example.com/ocpp");
    EXPECT_NO_THROW(config.validate());
    
    // Invalid reconnect interval
    config.setReconnectInterval(0);
    EXPECT_THROW(config.validate(), ConfigValidationError);
}

TEST_F(ConfigTest, DeviceConfigLoadFromYaml) {
    // Create test YAML file for Modbus TCP device
    std::string yaml_content = R"(
device:
  id: "CP001"
  template: "modbus_tcp_generic"
  protocol: "modbus_tcp"
  ocpp_id: "CP001"
  connection:
    ip: "192.168.1.100"
    port: 502
    unit_id: 1
)";
    
    std::string yaml_path = (temp_dir_ / "device_modbus_tcp.yaml").string();
    createYamlFile(yaml_path, yaml_content);
    
    // Load configuration from YAML file
    DeviceConfig config;
    EXPECT_TRUE(config.loadFromYaml(yaml_path));
    
    // Verify loaded values
    EXPECT_EQ(config.getId(), "CP001");
    EXPECT_EQ(config.getTemplateId(), "modbus_tcp_generic");
    EXPECT_EQ(config.getProtocol(), ProtocolType::MODBUS_TCP);
    EXPECT_EQ(config.getOcppId(), "CP001");
    
    // Verify connection
    const auto& connection = config.getConnection();
    EXPECT_TRUE(std::holds_alternative<ModbusTcpConnectionConfig>(connection));
    const auto& modbus_tcp = std::get<ModbusTcpConnectionConfig>(connection);
    EXPECT_EQ(modbus_tcp.ip, "192.168.1.100");
    EXPECT_EQ(modbus_tcp.port, 502);
    EXPECT_EQ(modbus_tcp.unit_id, 1);
    
    // Create test YAML file for ECHONET Lite device
    yaml_content = R"(
device:
  id: "CP002"
  template: "echonet_lite_generic"
  protocol: "echonet_lite"
  ocpp_id: "CP002"
  connection:
    ip: "192.168.1.101"
)";
    
    yaml_path = (temp_dir_ / "device_echonet_lite.yaml").string();
    createYamlFile(yaml_path, yaml_content);
    
    // Load configuration from YAML file
    DeviceConfig config2;
    EXPECT_TRUE(config2.loadFromYaml(yaml_path));
    
    // Verify loaded values
    EXPECT_EQ(config2.getId(), "CP002");
    EXPECT_EQ(config2.getTemplateId(), "echonet_lite_generic");
    EXPECT_EQ(config2.getProtocol(), ProtocolType::ECHONET_LITE);
    EXPECT_EQ(config2.getOcppId(), "CP002");
    
    // Verify connection
    const auto& connection2 = config2.getConnection();
    EXPECT_TRUE(std::holds_alternative<EchonetLiteConnectionConfig>(connection2));
    const auto& echonet_lite = std::get<EchonetLiteConnectionConfig>(connection2);
    EXPECT_EQ(echonet_lite.ip, "192.168.1.101");
}

TEST_F(ConfigTest, DeviceConfigValidation) {
    // Valid Modbus TCP device
    ModbusTcpConnectionConfig modbus_tcp;
    modbus_tcp.ip = "192.168.1.100";
    modbus_tcp.port = 502;
    modbus_tcp.unit_id = 1;
    
    DeviceConfig config("CP001", "modbus_tcp_generic", ProtocolType::MODBUS_TCP, modbus_tcp, "CP001");
    EXPECT_NO_THROW(config.validate());
    
    // Invalid ID
    DeviceConfig invalid_config("", "modbus_tcp_generic", ProtocolType::MODBUS_TCP, modbus_tcp, "CP001");
    EXPECT_THROW(invalid_config.validate(), ConfigValidationError);
    
    // Invalid connection
    ModbusTcpConnectionConfig invalid_modbus_tcp;
    invalid_modbus_tcp.ip = "";
    invalid_modbus_tcp.port = 502;
    invalid_modbus_tcp.unit_id = 1;
    
    DeviceConfig invalid_conn_config("CP001", "modbus_tcp_generic", ProtocolType::MODBUS_TCP, invalid_modbus_tcp, "CP001");
    EXPECT_THROW(invalid_conn_config.validate(), ConfigValidationError);
}

TEST_F(ConfigTest, DeviceConfigCollectionLoadFromYaml) {
    // Create test YAML file with multiple devices
    std::string yaml_content = R"(
devices:
  - id: "CP001"
    template: "modbus_tcp_generic"
    protocol: "modbus_tcp"
    ocpp_id: "CP001"
    connection:
      ip: "192.168.1.100"
      port: 502
      unit_id: 1
  - id: "CP002"
    template: "echonet_lite_generic"
    protocol: "echonet_lite"
    ocpp_id: "CP002"
    connection:
      ip: "192.168.1.101"
)";
    
    std::string yaml_path = (temp_dir_ / "devices.yaml").string();
    createYamlFile(yaml_path, yaml_content);
    
    // Load configuration from YAML file
    DeviceConfigCollection collection(yaml_path);
    
    // Verify loaded values
    EXPECT_EQ(collection.getDevices().size(), 2);
    
    // Get device by ID
    auto device1 = collection.getDevice("CP001");
    EXPECT_TRUE(device1.has_value());
    EXPECT_EQ(device1->getId(), "CP001");
    EXPECT_EQ(device1->getProtocol(), ProtocolType::MODBUS_TCP);
    
    auto device2 = collection.getDevice("CP002");
    EXPECT_TRUE(device2.has_value());
    EXPECT_EQ(device2->getId(), "CP002");
    EXPECT_EQ(device2->getProtocol(), ProtocolType::ECHONET_LITE);
    
    // Get non-existent device
    auto device3 = collection.getDevice("CP003");
    EXPECT_FALSE(device3.has_value());
}

TEST_F(ConfigTest, DeviceConfigCollectionAddRemove) {
    DeviceConfigCollection collection;
    
    // Add device
    ModbusTcpConnectionConfig modbus_tcp;
    modbus_tcp.ip = "192.168.1.100";
    modbus_tcp.port = 502;
    modbus_tcp.unit_id = 1;
    
    DeviceConfig device1("CP001", "modbus_tcp_generic", ProtocolType::MODBUS_TCP, modbus_tcp, "CP001");
    collection.addDevice(device1);
    
    // Verify device was added
    EXPECT_EQ(collection.getDevices().size(), 1);
    auto device = collection.getDevice("CP001");
    EXPECT_TRUE(device.has_value());
    EXPECT_EQ(device->getId(), "CP001");
    
    // Add another device
    EchonetLiteConnectionConfig echonet_lite;
    echonet_lite.ip = "192.168.1.101";
    
    DeviceConfig device2("CP002", "echonet_lite_generic", ProtocolType::ECHONET_LITE, echonet_lite, "CP002");
    collection.addDevice(device2);
    
    // Verify device was added
    EXPECT_EQ(collection.getDevices().size(), 2);
    
    // Remove device
    EXPECT_TRUE(collection.removeDevice("CP001"));
    EXPECT_EQ(collection.getDevices().size(), 1);
    EXPECT_FALSE(collection.getDevice("CP001").has_value());
    
    // Remove non-existent device
    EXPECT_FALSE(collection.removeDevice("CP003"));
    EXPECT_EQ(collection.getDevices().size(), 1);
}

TEST_F(ConfigTest, ConfigManagerInitialize) {
    // Initialize config manager
    ConfigManager& manager = ConfigManager::getInstance();
    EXPECT_TRUE(manager.initialize(temp_dir_.string()));
    
    // Create system config file
    std::string system_yaml = R"(
system:
  log_level: DEBUG
  log_rotation:
    max_size_mb: 20
    max_files: 10
  metrics:
    prometheus_port: 8080
  security:
    tls_cert_path: "/path/to/cert.pem"
    tls_key_path: "/path/to/key.pem"
    ca_cert_path: "/path/to/ca.pem"
    client_cert_required: true
)";
    
    createYamlFile((temp_dir_ / "system.yaml").string(), system_yaml);
    
    // Create CSMS config file
    std::string csms_yaml = R"(
csms:
  url: "wss://test-csms.example.com/ocpp"
  reconnect_interval_sec: 60
  max_reconnect_attempts: 5
  heartbeat_interval_sec: 600
)";
    
    createYamlFile((temp_dir_ / "csms.yaml").string(), csms_yaml);
    
    // Create device config file
    std::string device_yaml = R"(
devices:
  - id: "CP001"
    template: "modbus_tcp_generic"
    protocol: "modbus_tcp"
    ocpp_id: "CP001"
    connection:
      ip: "192.168.1.100"
      port: 502
      unit_id: 1
)";
    
    createYamlFile((temp_dir_ / "devices/device1.yaml").string(), device_yaml);
    
    // Reload configs
    EXPECT_TRUE(manager.reloadAllConfigs());
    
    // Verify loaded values
    const SystemConfig& system_config = manager.getSystemConfig();
    EXPECT_EQ(system_config.getLogLevel(), LogLevel::DEBUG);
    
    const CsmsConfig& csms_config = manager.getCsmsConfig();
    EXPECT_EQ(csms_config.getUrl(), "wss://test-csms.example.com/ocpp");
    
    auto device = manager.getDeviceConfig("CP001");
    EXPECT_TRUE(device.has_value());
    EXPECT_EQ(device->getId(), "CP001");
}

TEST_F(ConfigTest, ConfigManagerAddRemoveDevice) {
    // Initialize config manager
    ConfigManager& manager = ConfigManager::getInstance();
    EXPECT_TRUE(manager.initialize(temp_dir_.string()));
    
    // Add device
    ModbusTcpConnectionConfig modbus_tcp;
    modbus_tcp.ip = "192.168.1.100";
    modbus_tcp.port = 502;
    modbus_tcp.unit_id = 1;
    
    DeviceConfig device("CP001", "modbus_tcp_generic", ProtocolType::MODBUS_TCP, modbus_tcp, "CP001");
    EXPECT_TRUE(manager.addOrUpdateDeviceConfig(device));
    
    // Verify device was added
    auto device_opt = manager.getDeviceConfig("CP001");
    EXPECT_TRUE(device_opt.has_value());
    EXPECT_EQ(device_opt->getId(), "CP001");
    
    // Remove device
    EXPECT_TRUE(manager.removeDeviceConfig("CP001"));
    EXPECT_FALSE(manager.getDeviceConfig("CP001").has_value());
}

} // namespace test
} // namespace config
} // namespace ocpp_gateway