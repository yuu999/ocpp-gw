# OCPP 2.0.1 Gateway Middleware 開発者ガイド

## 概要

本ドキュメントは、OCPP 2.0.1 Gateway Middlewareの開発者向けガイドです。アーキテクチャ、開発環境、API仕様、テスト方法について説明します。

## アーキテクチャ

### システム構成
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   CSMS Server   │◄──►│  OCPP Gateway   │◄──►│  Charging       │
│                 │    │   Middleware    │    │  Stations       │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                              │
                              ▼
                       ┌─────────────────┐
                       │   Admin API     │
                       │   Web UI/CLI    │
                       └─────────────────┘
```

### 主要コンポーネント

#### 1. OCPP Client Manager
- WebSocket接続管理
- OCPP 2.0.1プロトコル実装
- メッセージシリアライゼーション/デシリアライゼーション
- 状態管理

#### 2. Mapping Engine
- プロトコル変換
- 設定ファイル管理
- ホットリロード機能
- テンプレート機能

#### 3. Device Adapters
- ECHONET Lite Adapter
- Modbus RTU Adapter
- Modbus TCP Adapter
- 共通インターフェース

#### 4. Admin API
- REST API
- CLI
- Web UI
- 認証・認可

## 開発環境セットアップ

### 1. 必要なツール
```bash
# 基本ツール
sudo apt install build-essential cmake git

# 依存ライブラリ
sudo apt install libboost-all-dev libssl-dev libyaml-cpp-dev
sudo apt install libjsoncpp-dev libspdlog-dev libmodbus-dev

# 開発ツール
sudo apt install clang-format clang-tidy cppcheck
sudo apt install valgrind gdb
```

### 2. プロジェクトクローン
```bash
git clone https://github.com/your-repo/ocpp-gateway.git
cd ocpp-gateway
```

### 3. ビルド
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 4. テスト実行
```bash
# 単体テスト
make test

# 統合テスト
make integration_test

# カバレッジテスト
make coverage_test
```

## コーディング規約

### 1. 命名規則
```cpp
// クラス名: PascalCase
class OcppClientManager {};

// メソッド名: camelCase
void connectToCsms();

// 変数名: snake_case
std::string device_id;

// 定数名: UPPER_SNAKE_CASE
const int MAX_DEVICES = 100;

// 名前空間: snake_case
namespace ocpp_gateway {
namespace common {
```

### 2. ファイル構成
```
src/
├── common/           # 共通ライブラリ
├── api/             # API関連
├── device/          # デバイスアダプタ
├── ocpp/            # OCPP関連
└── main.cpp         # メインエントリーポイント
```

### 3. エラーハンドリング
```cpp
// 例外を使用
class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& message)
        : std::runtime_error(message) {}
};

// エラーコードを使用
enum class ErrorCode {
    SUCCESS = 0,
    INVALID_CONFIG = 1,
    NETWORK_ERROR = 2,
    // ...
};
```

### 4. ログ出力
```cpp
#include "ocpp_gateway/common/logger.h"

LOG_INFO("Device connected: {}", device_id);
LOG_ERROR("Connection failed: {}", error_message);
LOG_DEBUG("Processing message: {}", message_type);
```

## API設計

### 1. REST API設計原則
- RESTful設計
- JSON形式でのデータ交換
- HTTPステータスコードの適切な使用
- バージョニング（/api/v1/）

### 2. エンドポイント例
```cpp
// デバイス管理
GET    /api/v1/devices
POST   /api/v1/devices
GET    /api/v1/devices/{id}
PUT    /api/v1/devices/{id}
DELETE /api/v1/devices/{id}

// 設定管理
GET    /api/v1/config
PUT    /api/v1/config
POST   /api/v1/config/reload

// 監視
GET    /api/v1/status
GET    /api/v1/metrics
GET    /api/v1/logs
```

### 3. レスポンス形式
```json
{
  "success": true,
  "data": {
    "device_id": "CP001",
    "status": "online",
    "last_seen": "2024-12-21T10:00:00Z"
  },
  "error": null,
  "timestamp": "2024-12-21T10:00:00Z"
}
```

## テスト戦略

### 1. 単体テスト
```cpp
#include <gtest/gtest.h>

class OcppClientManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // テスト前のセットアップ
    }
    
    void TearDown() override {
        // テスト後のクリーンアップ
    }
};

TEST_F(OcppClientManagerTest, ConnectToCsms) {
    // テストケース
    EXPECT_TRUE(manager.connectToCsms("wss://test.com"));
}
```

### 2. 統合テスト
```cpp
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // テスト環境のセットアップ
        startMockCsms();
        startMockDevice();
    }
};

TEST_F(IntegrationTest, CompleteTransaction) {
    // エンドツーエンドテスト
    // 1. デバイス接続
    // 2. OCPP接続
    // 3. トランザクション開始
    // 4. メータ値送信
    // 5. トランザクション終了
}
```

### 3. パフォーマンステスト
```cpp
TEST(PerformanceTest, HundredDevices) {
    // 100台のデバイスをシミュレート
    // CPU使用率、メモリ使用量、レスポンス時間を測定
}
```

## デバッグ・プロファイリング

### 1. デバッグ方法
```bash
# GDBを使用したデバッグ
gdb --args ./ocpp-gateway --config /path/to/config

# Valgrindを使用したメモリチェック
valgrind --leak-check=full ./ocpp-gateway

# AddressSanitizer
cmake -DCMAKE_BUILD_TYPE=Debug -DSANITIZE=address ..
```

### 2. プロファイリング
```bash
# perfを使用したプロファイリング
perf record ./ocpp-gateway
perf report

# gprofを使用したプロファイリング
cmake -DCMAKE_BUILD_TYPE=Debug -DPROFILE=ON ..
```

## セキュリティ開発

### 1. TLS実装
```cpp
#include "ocpp_gateway/common/tls_manager.h"

TlsManager tls_manager;
tls_manager.loadConfiguration(security_config);

if (tls_manager.isTlsEnabled()) {
    // TLS接続の確立
    auto context = tls_manager.getTlsContext();
    // ...
}
```

### 2. RBAC実装
```cpp
#include "ocpp_gateway/common/rbac_manager.h"

RbacManager rbac_manager;
rbac_manager.initialize();

// 認証
auto result = rbac_manager.authenticate(username, password);
if (result.success) {
    // 認可チェック
    if (rbac_manager.hasPermission(username, Permission::DEVICE_READ)) {
        // アクセス許可
    }
}
```

### 3. 入力検証
```cpp
// 入力値の検証
bool validateDeviceId(const std::string& device_id) {
    // 正規表現による検証
    std::regex pattern("^[A-Z0-9]{3,10}$");
    return std::regex_match(device_id, pattern);
}
```

## 国際化対応

### 1. i18n実装
```cpp
#include "ocpp_gateway/common/i18n_manager.h"

I18nManager i18n_manager;
i18n_manager.initialize();
i18n_manager.setLanguage(Language::JAPANESE);

// 翻訳テキスト取得
std::string message = i18n_manager.getText("device.connected");
```

### 2. 翻訳ファイル形式
```json
{
  "system.startup": "OCPPゲートウェイを起動中...",
  "device.connected": "デバイスが接続されました",
  "error.network": "ネットワークエラー"
}
```

## パッケージング

### 1. Debianパッケージ作成
```bash
# パッケージビルド
dpkg-buildpackage -b -us -uc

# パッケージインストール
sudo dpkg -i ocpp-gateway_1.0.0_amd64.deb
```

### 2. systemdサービス設定
```ini
[Unit]
Description=OCPP Gateway Middleware
After=network.target

[Service]
Type=simple
User=ocpp-gateway
ExecStart=/usr/bin/ocpp-gateway --config /etc/ocpp-gateway/
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

## CI/CD

### 1. GitHub Actions設定
```yaml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Install dependencies
      run: |
        sudo apt update
        sudo apt install -y build-essential cmake libboost-all-dev
    - name: Build
      run: |
        mkdir build && cd build
        cmake ..
        make -j$(nproc)
    - name: Test
      run: |
        make test
```

### 2. コード品質チェック
```bash
# clang-format
find . -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# clang-tidy
clang-tidy src/*.cpp -- -Iinclude

# cppcheck
cppcheck --enable=all src/
```

## ドキュメント生成

### 1. Doxygen設定
```bash
# Doxygen設定ファイル生成
doxygen -g

# ドキュメント生成
doxygen Doxyfile
```

### 2. API仕様書生成
```bash
# OpenAPI仕様書生成
swagger-codegen generate -i api_spec.yaml -l html2
```

## リリース管理

### 1. バージョニング
- セマンティックバージョニング（MAJOR.MINOR.PATCH）
- 例: 1.0.0, 1.1.0, 1.1.1

### 2. 変更履歴
```markdown
# Changelog

## [1.1.0] - 2024-12-21
### Added
- TLS 1.3対応
- RBAC機能
- 国際化対応

### Changed
- パフォーマンス改善
- ログ出力形式変更

### Fixed
- メモリリーク修正
- 接続エラー修正
```

## トラブルシューティング

### 1. よくある問題

#### ビルドエラー
```bash
# 依存関係確認
ldd ./ocpp-gateway

# ライブラリパス確認
echo $LD_LIBRARY_PATH
```

#### 実行時エラー
```bash
# スタックトレース確認
gdb ./ocpp-gateway core

# ログ確認
tail -f /var/log/ocpp-gateway/ocpp-gateway.log
```

### 2. デバッグTips
- ログレベルをDEBUGに設定
- 特定コンポーネントのログを有効化
- メモリ使用量の監視
- ネットワーク接続の確認

## 貢献ガイド

### 1. プルリクエスト
1. フォークを作成
2. 機能ブランチを作成
3. 変更を実装
4. テストを追加
5. プルリクエストを作成

### 2. コードレビュー
- 機能性の確認
- セキュリティの確認
- パフォーマンスの確認
- ドキュメントの更新

### 3. テスト要件
- 単体テストの追加
- 統合テストの追加
- パフォーマンステストの追加

## ライセンス

本プロジェクトはMITライセンスの下で提供されています。詳細は[LICENSE](../LICENSE)ファイルを参照してください。 