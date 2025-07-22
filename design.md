# OCPP 2.0.1 ゲートウェイ設計文書

## 1. システム概要

### 1.1 アーキテクチャ概要

```
[CSMS] ⇄ TLS/WebSocket ⇄ [OCPP Gateway] ⇄ EL/Modbus ⇄ [充電器群]
                              ┌─────────────────┐
                              │ OcppClientMgr   │
                              ├─────────────────┤
                              │ MappingEngine   │
                              ├─────────────────┤
                              │ DeviceAdapters  │
                              │ ├─ ElAdapter    │
                              │ └─ ModbusAdapter│
                              ├─────────────────┤
                              │ AdminAPI        │
                              └─────────────────┘
```

### 1.2 主要コンポーネント

| コンポーネント | 役割 | 実装技術 |
|-------------|------|----------|
| **OcppClientManager** | OCPP WebSocket接続管理、JSONメッセージ処理 | Boost.Asio, websocketpp |
| **MappingEngine** | OCPP変数⇄デバイス値の変換マッピング | YAML解析、動的設定 |
| **ElAdapter** | ECHONET Lite通信処理 | UDP socket, EPC処理 |
| **ModbusAdapter** | Modbus RTU/TCP通信処理 | libmodbus |
| **AdminAPI** | 管理インタフェース | RESTful API |

## 2. 詳細設計

### 2.1 OcppClientManager

```cpp
class OcppClientManager {
public:
    // WebSocket接続管理
    bool connectToCSMS(const std::string& url, const TlsConfig& tls);
    void disconnect();
    
    // メッセージ送受信
    void sendMessage(const std::string& chargePointId, const Json::Value& message);
    void handleIncomingMessage(const std::string& message);
    
    // 充電ポイント管理
    void addChargePoint(const std::string& id, const ChargePointConfig& config);
    void removeChargePoint(const std::string& id);
    
private:
    websocketpp::client<websocketpp::config::asio_tls_client> wsClient_;
    std::map<std::string, ChargePointInstance> chargePoints_;
    boost::asio::io_context ioContext_;
};
```

### 2.2 MappingEngine

```cpp
class MappingEngine {
public:
    // マッピング設定の読み込み
    bool loadMappingConfig(const std::string& configPath);
    void reloadConfig();
    
    // OCPP⇄デバイス値変換
    Json::Value deviceToOcpp(const std::string& deviceId, 
                             const std::string& variable,
                             const DeviceValue& value);
    DeviceValue ocppToDevice(const std::string& deviceId,
                            const std::string& variable,
                            const Json::Value& ocppValue);
    
    // テンプレート管理
    bool loadDeviceTemplate(const std::string& vendor, const std::string& model);
    
private:
    std::map<std::string, DeviceTemplate> templates_;
    std::map<std::string, DeviceMappingConfig> deviceConfigs_;
    YAML::Node mappingConfig_;
};
```

### 2.3 DeviceAdapter基底クラス

```cpp
class DeviceAdapter {
public:
    virtual ~DeviceAdapter() = default;
    
    // デバイス通信
    virtual bool connect(const DeviceConfig& config) = 0;
    virtual void disconnect() = 0;
    virtual DeviceValue readValue(const std::string& address) = 0;
    virtual bool writeValue(const std::string& address, const DeviceValue& value) = 0;
    
    // 状態監視
    virtual DeviceStatus getStatus() const = 0;
    virtual void startMonitoring() = 0;
    virtual void stopMonitoring() = 0;
    
protected:
    DeviceConfig config_;
    DeviceStatus status_;
    std::function<void(const std::string&, const DeviceValue&)> valueChangeCallback_;
};
```

### 2.4 ElAdapter

```cpp
class ElAdapter : public DeviceAdapter {
public:
    // ECHONET Lite 固有実装
    bool connect(const DeviceConfig& config) override;
    DeviceValue readValue(const std::string& epc) override;
    bool writeValue(const std::string& epc, const DeviceValue& value) override;
    
    // EL固有機能
    void discoverDevices();
    bool sendEchonetFrame(const EchonetFrame& frame);
    
private:
    boost::asio::ip::udp::socket udpSocket_;
    std::map<std::string, EchonetDevice> discoveredDevices_;
    
    void handleMulticastResponse(const boost::system::error_code& error,
                                std::size_t bytes_transferred);
};
```

### 2.5 ModbusAdapter

```cpp
class ModbusAdapter : public DeviceAdapter {
public:
    // Modbus 固有実装
    bool connect(const DeviceConfig& config) override;
    DeviceValue readValue(const std::string& register_addr) override;
    bool writeValue(const std::string& register_addr, const DeviceValue& value) override;
    
    // Modbus固有機能
    bool connectTCP(const std::string& host, int port);
    bool connectRTU(const std::string& device, int baud, char parity, int data_bit, int stop_bit);
    
private:
    modbus_t* modbusContext_;
    ModbusConnectionType connectionType_;
    
    bool readHoldingRegisters(int addr, int nb, uint16_t* dest);
    bool writeMultipleRegisters(int addr, int nb, const uint16_t* src);
};
```

## 3. データ構造

### 3.1 設定ファイル構造

#### system_config.yaml
```yaml
gateway:
  id: "OCPP_GW_001"
  csms_url: "wss://csms.example.com:443/ocpp"
  max_charge_points: 100
  
tls:
  ca_cert_path: "/etc/ocpp-gw/certs/ca.pem"
  client_cert_path: "/etc/ocpp-gw/certs/client.pem"
  client_key_path: "/etc/ocpp-gw/certs/client.key"

devices:
  - id: "CP001"
    name: "充電器1号機"
    protocol: "modbus_tcp"
    host: "192.168.1.100"
    port: 502
    slave_id: 1
    template: "vendor_a_model_x"
    
  - id: "CP002"
    name: "充電器2号機"
    protocol: "echonet_lite"
    ip: "192.168.1.101"
    template: "vendor_b_model_y"
```

#### device_template/vendor_a_model_x.yaml
```yaml
metadata:
  vendor: "Vendor A"
  model: "Model X"
  version: "1.0"

mappings:
  # OCPP Variable → Modbus Register
  "AvailabilityState":
    type: "modbus"
    register: 40001
    data_type: "uint16"
    scale: 1
    enum:
      0: "Available"
      1: "Occupied"
      2: "Faulted"
      
  "Energy.Active.Import.Register":
    type: "modbus"
    register: 40010
    data_type: "uint32"
    scale: 0.1
    unit: "kWh"
    
  "Power.Active.Import":
    type: "modbus"
    register: 40012
    data_type: "uint16"
    scale: 0.1
    unit: "kW"
```

### 3.2 内部データ構造

```cpp
struct DeviceValue {
    enum Type { INTEGER, DOUBLE, STRING, BOOLEAN } type;
    union {
        int64_t intValue;
        double doubleValue;
        bool boolValue;
    };
    std::string stringValue;
    std::chrono::system_clock::time_point timestamp;
};

struct ChargePointConfig {
    std::string id;
    std::string name;
    std::string deviceId;
    int connectorCount;
    std::vector<std::string> supportedProfiles;
};

struct DeviceConfig {
    std::string id;
    std::string protocol; // "modbus_tcp", "modbus_rtu", "echonet_lite"
    std::string templateName;
    std::map<std::string, std::string> parameters; // host, port, slave_id等
};
```

## 4. 通信フロー

### 4.1 起動時シーケンス

```
1. 設定ファイル読み込み (system_config.yaml)
2. デバイステンプレート読み込み (device_template/*.yaml)
3. デバイスアダプタ初期化・接続
4. OCPP ClientManager初期化
5. CSMS接続・BootNotification送信
6. 定期監視タスク開始
```

### 4.2 値変更検知・通知フロー

```
デバイス値変更 → DeviceAdapter → MappingEngine → OcppClientManager → CSMS
      ↑                                    ↓
   ポーリング/イベント                StatusNotification/MeterValues
```

### 4.3 リモート操作フロー

```
CSMS → OcppClientManager → MappingEngine → DeviceAdapter → 充電器
   RemoteStartTransaction     値変換      WriteRegister/EPC
```

## 5. エラーハンドリング

### 5.1 通信エラー
- **CSMS切断**: オフラインキューイング、再接続時自動送信
- **デバイス無応答**: 設定回数リトライ後Faulted状態通知
- **プロトコルエラー**: ログ記録、上位レイヤーへエラー通知

### 5.2 設定エラー
- **不正なマッピング**: 起動時検証、エラー時はデフォルト値使用
- **デバイステンプレート不正**: 該当デバイスを無効化

## 6. ログ設計

```cpp
enum LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

class Logger {
public:
    static void log(LogLevel level, const std::string& category, 
                   const std::string& message);
    static void setLevel(LogLevel level);
    static void enableRotation(size_t maxSize, int maxFiles);
    
private:
    static std::shared_ptr<spdlog::logger> logger_;
};

// 使用例
Logger::log(LogLevel::INFO, "OCPP", "BootNotification sent");
Logger::log(LogLevel::ERROR, "Modbus", "Failed to read register 40001");
```

## 7. セキュリティ設計

### 7.1 TLS設定
- TLS 1.2以上を使用
- サーバ証明書検証必須
- クライアント証明書はオプション

### 7.2 設定ファイル保護
- 証明書・秘密鍵は権限制限
- 設定ファイルのハッシュ検証

## 8. 性能設計

### 8.1 非同期処理
- Boost.Asioを使用した非同期I/O
- WebSocket接続プールでスケーラビリティ確保

### 8.2 メモリ管理
- スマートポインター活用でメモリリーク防止
- オブジェクトプール使用でアロケーション最適化

### 8.3 スレッド設計
- I/Oスレッド: WebSocket/UDP/シリアル通信
- ワーカースレッド: データ変換・ビジネスロジック
- 管理スレッド: 設定再読み込み・ヘルスチェック 

## 9. ソースコードディレクトリ構成

```text
.
├── include/                # Public C++ headers
│   └── ocpp_gateway/       # Gateway modules grouped by domain
│       ├── api/            # REST, CLI, and Web UI interface definitions
│       ├── common/         # Core utilities (logging, config, TLS, etc.)
│       ├── device/         # Device adapter abstractions (EL / Modbus)
│       ├── mapping/        # Variable mapping engine
│       └── ocpp/           # OCPP 2.0.1 protocol implementation
├── src/                    # Implementations corresponding to headers
│   └── ...                 # Mirrors the structure of include/
├── config/                 # YAML configuration (system, csms, devices, templates)
├── docs/                   # User & developer documentation (this file, guides)
├── tests/                  # Unit & integration test suites
│   ├── unit/               # Component-level tests
│   └── integration/        # End-to-end scenario tests
├── examples/               # Minimal runnable code samples
├── scripts/                # Service unit files and helper scripts
├── work_log/               # Chronological development logs
└── third_party/            # External libraries (when vendored)
``` 