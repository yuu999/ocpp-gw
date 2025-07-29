# ECHONET Lite Simulator

OCPPゲートウェイのテスト環境として開発されたECHONET Liteシミュレータです。
EV充電器（0x02A1）およびEV充放電器（0x027E）クラスを模擬し、自動テスト環境を構築します。

## 機能

- **EV充電器（0x02A1）**および**EV充放電器（0x027E）**クラスの模擬
- 積算電力量リセット機能（EPC 0xD9/0xD7）
- 時間経過での自動値更新（シミュレーションモード）
- 複数インスタンス同時起動
- JSON/YAML設定ファイルによるインスタンス定義
- REST APIによる制御
- CLIコマンドによる制御
- Dockerコンテナ対応

## 要件

- Node.js 18以上
- Docker（オプション）

## インストール

### ローカル環境

```bash
git clone <repository>
cd echonet-simulator
npm install
npm run build
```

### Docker環境

```bash
docker build -t echonet-simulator .
docker run -p 3610:3610/udp -p 8080:8080 echonet-simulator
```

### Docker Compose

```bash
docker-compose up -d
```

## 使用方法

### 起動

```bash
# 開発モード
npm run dev

# 本番モード
npm start
```

### 設定

`config/default.yaml`ファイルでデバイス設定を変更できます：

```yaml
devices:
  - eoj: "0x02A1"  # EV Charger
    instance: 1
    name: "EV Charger 1"
    properties:
      - epc: 0xD8  # 積算充電電力量
        initialValue: 0
        simulation:
          type: "increment"
          interval: 5000  # ms
          step: 1000      # Wh
```

### REST API

#### デバイス一覧取得
```bash
curl http://localhost:8080/api/v1/devices
```

#### プロパティ値取得
```bash
curl http://localhost:8080/api/v1/devices/0x02A1:1/properties/0xD8
```

#### プロパティ値設定
```bash
curl -X PUT http://localhost:8080/api/v1/devices/0x02A1:1/properties/0xD9 \
  -H "Content-Type: application/json" \
  -d '{"value": "00"}'
```

### CLIコマンド

```bash
# プロパティ値設定
npm run cli set 0x02A1:1 0xD9 0x00

# プロパティ値取得
npm run cli get 0x02A1:1 0xD8
```

## 開発

### プロジェクト構造

```
echonet-simulator/
├── src/
│   ├── core/           # ECHONET Lite基本実装
│   ├── classes/        # デバイスクラス実装
│   ├── api/           # REST API
│   ├── cli/           # CLIコマンド
│   └── types/         # 型定義
├── config/            # 設定ファイル
├── tests/             # テストコード
├── docker/            # Docker設定
└── docs/              # ドキュメント
```

### テスト

```bash
npm test
```

### ビルド

```bash
npm run build
```

## OCPPゲートウェイとの統合

1. **シミュレータ起動**
   ```bash
   docker-compose up echonet-simulator
   ```

2. **OCPPゲートウェイ設定**
   - ECHONET転送先IPをシミュレータのホストに設定
   - ポート3610でUDP通信

3. **テスト実行**
   ```bash
   # リセットコマンド送信
   curl -X PUT http://localhost:8080/api/v1/devices/0x02A1:1/properties/0xD9 \
     -H "Content-Type: application/json" \
     -d '{"value": "00"}'
   
   # 値確認
   curl http://localhost:8080/api/v1/devices/0x02A1:1/properties/0xD8
   ```

## ライセンス

MIT License

## 貢献

プルリクエストやイシューの報告を歓迎します。

## 参考

- [ECHONET Lite規格](https://echonet.jp/spec_g/)
- [elemu - ECHONET Lite機器エミュレータ](https://github.com/KAIT-HEMS/elemu)
- [OCPP 2.0.1 Specification](https://www.openchargealliance.org/protocols/ocpp-201/) 