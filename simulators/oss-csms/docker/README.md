# Docker-based OCPP CSMS

Docker Composeを使用してOCPP CSMSとOCPP Gatewayの統合テスト環境を構築します。

## 構成

- **CSMS**: ocpp-cs/csms (OCPP 1.5/1.6対応)
- **OCPP Gateway**: メインアプリケーション
- **ECHONET Simulator**: ECHONET Lite充電器シミュレータ

## 使用方法

### 1. 起動

```bash
cd simulators/oss-csms/docker
docker-compose up -d
```

### 2. アクセス

- **CSMS Web UI**: http://localhost:8080
- **OCPP Gateway API**: http://localhost:8081
- **ECHONET Simulator**: UDP 3610

### 3. ログ確認

```bash
# CSMSログ
docker-compose logs csms

# OCPP Gatewayログ
docker-compose logs ocpp-gateway

# ECHONET Simulatorログ
docker-compose logs echonet-simulator
```

### 4. 停止

```bash
docker-compose down
```

## 設定

### CSMS設定
`config/csms.yaml`でCSMSの設定を変更できます。

### OCPP Gateway設定
`config/gateway.yaml`でOCPP Gatewayの設定を変更できます。

## テスト

1. **ECHONET充電器からの接続**
   - ECHONET SimulatorがUDP 3610でリッスン
   - OCPP GatewayがECHONETメッセージを受信

2. **OCPP CSMSへの接続**
   - OCPP GatewayがCSMSにWebSocket接続
   - 充電器情報がCSMSに登録

3. **トランザクション処理**
   - 充電開始/終了のテスト
   - メーター値の送信テスト

## トラブルシューティング

### ポート競合
```bash
# 使用中のポートを確認
netstat -tuln | grep -E ':(9000|8080|8081|3610)'

# 競合するプロセスを停止
sudo lsof -ti:9000 | xargs kill -9
```

### ログ確認
```bash
# 全サービスのログ
docker-compose logs

# 特定サービスのログ（フォロー）
docker-compose logs -f csms
``` 