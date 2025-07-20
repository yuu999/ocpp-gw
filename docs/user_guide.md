# OCPP 2.0.1 Gateway Middleware ユーザーガイド

## 概要

OCPP 2.0.1 Gateway Middlewareは、既存のECHONET Lite（EL）またはModbus（RTU/TCP）対応充電器をOCPP 2.0.1ネットワークに統合するためのゲートウェイ・ミドルウェアです。最大100台の充電器を一括管理でき、充電器側のファームウェア変更なしでOCPP 2.0.1対応を実現します。

## 主な機能

### 1. OCPP 2.0.1 Core Profile対応
- BootNotification、Heartbeat、StatusNotification
- TransactionEvent、Authorize
- RemoteStart/Stop Transaction
- MeterValues送信（取引中1分間隔）
- SetChargingProfile（電流上限設定）

### 2. プロトコル変換
- ECHONET Lite ↔ OCPP 2.0.1
- Modbus RTU/TCP ↔ OCPP 2.0.1
- 外部定義ファイルによる動的マッピング
- テンプレート機能による設定簡素化

### 3. セキュリティ機能
- TLS 1.2/1.3対応
- 証明書管理
- RBAC（Role-Based Access Control）
- 認証・認可機能

### 4. 監視・管理機能
- Web UIによる管理画面
- CLIによるコマンドライン管理
- Prometheusメトリクス出力
- ログ管理・ローテーション

## システム要件

### ハードウェア要件
- CPU: ARMv8 1.6GHz以上（4コア推奨）
- メモリ: 4GB以上
- ストレージ: 16GB以上
- ネットワーク: イーサネット（100Mbps以上）

### ソフトウェア要件
- OS: Debian/Ubuntu 22.04、Yocto-based distro
- 依存ライブラリ: Boost、OpenSSL、yaml-cpp等

## インストール

### 1. パッケージインストール
```bash
# Debian/Ubuntu
sudo apt update
sudo apt install ocpp-gateway

# または、ソースからビルド
git clone https://github.com/your-repo/ocpp-gateway.git
cd ocpp-gateway
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### 2. 初期設定
```bash
# 設定ディレクトリ作成
sudo mkdir -p /etc/ocpp-gateway
sudo mkdir -p /var/log/ocpp-gateway
sudo mkdir -p /etc/ocpp-gateway/certs

# 設定ファイルコピー
sudo cp /usr/share/ocpp-gateway/config/* /etc/ocpp-gateway/
```

## 設定

### 1. システム設定（system.yaml）
```yaml
system:
  log_level: INFO
  log_rotation:
    max_size_mb: 10
    max_files: 5
  metrics:
    prometheus_port: 9090
  security:
    tls_cert_path: "/etc/ocpp-gateway/certs/server.crt"
    tls_key_path: "/etc/ocpp-gateway/certs/server.key"
    ca_cert_path: "/etc/ocpp-gateway/certs/ca.crt"
    client_cert_required: false
    tls_version: "1.2"
    verify_server_cert: true
    verify_client_cert: false
```

### 2. CSMS設定（csms.yaml）
```yaml
csms:
  url: "wss://csms.example.com/ocpp"
  reconnect_interval_sec: 30
  max_reconnect_attempts: 10
  heartbeat_interval_sec: 300
```

### 3. デバイス設定（devices/）
```yaml
# devices/sample_device.yaml
devices:
  - id: "CP001"
    template: "vendor_x_model_y"
    protocol: "modbus_tcp"
    connection:
      ip: "192.168.1.100"
      port: 502
      unit_id: 1
    ocpp_id: "CP001"
```

### 4. マッピングテンプレート（templates/）
```yaml
# templates/modbus_tcp_template.yaml
template:
  id: "vendor_x_model_y"
  description: "Vendor X Model Y Charger"
  variables:
    - ocpp_name: "AvailabilityState"
      type: "modbus"
      register: 40001
      data_type: "uint16"
      enum:
        0: "Available"
        1: "Occupied"
        2: "Reserved"
        3: "Unavailable"
        4: "Faulted"
    - ocpp_name: "MeterValue.Energy.Active.Import.Register"
      type: "modbus"
      register: 40010
      data_type: "float32"
      scale: 0.1
      unit: "kWh"
```

## 使用方法

### 1. サービス起動
```bash
# systemdサービス起動
sudo systemctl start ocpp-gateway
sudo systemctl enable ocpp-gateway

# 手動起動
ocpp-gateway --config /etc/ocpp-gateway/
```

### 2. Web UIアクセス
- URL: `https://localhost:8080`
- デフォルトユーザー: `admin`
- デフォルトパスワード: `admin`

### 3. CLI使用例
```bash
# システム状態確認
ocpp-gateway-cli status

# デバイス一覧表示
ocpp-gateway-cli devices list

# 設定再読み込み
ocpp-gateway-cli config reload

# マッピングテスト
ocpp-gateway-cli mapping test CP001
```

### 4. ログ確認
```bash
# ログファイル確認
tail -f /var/log/ocpp-gateway/ocpp-gateway.log

# ログレベル変更
ocpp-gateway-cli log level DEBUG
```

## セキュリティ設定

### 1. TLS証明書設定
```bash
# 自己署名証明書生成（開発用）
openssl req -x509 -newkey rsa:4096 -keyout server.key -out server.crt -days 365 -nodes

# 証明書配置
sudo cp server.crt /etc/ocpp-gateway/certs/
sudo cp server.key /etc/ocpp-gateway/certs/
sudo chmod 600 /etc/ocpp-gateway/certs/server.key
```

### 2. ユーザー管理
```bash
# ユーザー追加
ocpp-gateway-cli user add operator1 --role operator --password secure123

# パスワード変更
ocpp-gateway-cli user password operator1 --new-password newsecure123

# ユーザー削除
ocpp-gateway-cli user delete operator1
```

## 監視・メトリクス

### 1. Prometheusメトリクス
- エンドポイント: `http://localhost:9090/metrics`
- 主要メトリクス:
  - `ocpp_gateway_devices_total`
  - `ocpp_gateway_ocpp_messages_total`
  - `ocpp_gateway_transactions_total`

### 2. ヘルスチェック
```bash
# ヘルスチェック
curl http://localhost:8080/health

# 詳細状態確認
curl http://localhost:8080/api/v1/status
```

## トラブルシューティング

### 1. よくある問題

#### 接続エラー
```bash
# ログ確認
tail -f /var/log/ocpp-gateway/ocpp-gateway.log | grep ERROR

# ネットワーク確認
ping 192.168.1.100
telnet 192.168.1.100 502
```

#### 設定エラー
```bash
# 設定ファイル構文チェック
ocpp-gateway-cli config validate

# 設定ファイル再読み込み
ocpp-gateway-cli config reload
```

#### パフォーマンス問題
```bash
# システムリソース確認
top
htop
iotop

# メトリクス確認
curl http://localhost:9090/metrics | grep cpu
```

### 2. ログレベル調整
```bash
# デバッグログ有効化
ocpp-gateway-cli log level DEBUG

# 特定コンポーネントのログレベル設定
ocpp-gateway-cli log component ocpp DEBUG
ocpp-gateway-cli log component device DEBUG
```

### 3. 設定ファイルバックアップ・復元
```bash
# バックアップ
sudo tar -czf ocpp-gateway-config-backup.tar.gz /etc/ocpp-gateway/

# 復元
sudo tar -xzf ocpp-gateway-config-backup.tar.gz -C /
```

## アップデート

### 1. パッケージアップデート
```bash
# Debian/Ubuntu
sudo apt update
sudo apt upgrade ocpp-gateway

# サービス再起動
sudo systemctl restart ocpp-gateway
```

### 2. OTA更新
```bash
# 更新パッケージダウンロード
wget https://example.com/ocpp-gateway-update.tar.gz

# 更新実行
sudo ocpp-gateway-update /path/to/update.tar.gz
```

## サポート

### 1. ドキュメント
- [技術仕様書](technical_specification.md)
- [API仕様書](api_reference.md)
- [トラブルシューティングガイド](troubleshooting.md)

### 2. ログ収集
```bash
# 診断情報収集
ocpp-gateway-cli diagnose collect

# ログファイルアーカイブ
sudo tar -czf ocpp-gateway-logs.tar.gz /var/log/ocpp-gateway/
```

### 3. サポート連絡先
- メール: support@ocpp-gateway.com
- 電話: +81-XX-XXXX-XXXX
- オンラインサポート: https://support.ocpp-gateway.com

## ライセンス

本ソフトウェアはMITライセンスの下で提供されています。詳細は[LICENSE](../LICENSE)ファイルを参照してください。 