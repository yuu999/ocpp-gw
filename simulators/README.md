# OCPP Gateway Simulators

このディレクトリには、OCPP Gatewayのテスト用シミュレータが含まれています。

## ディレクトリ構成

```
simulators/
├── README.md                    # このファイル
├── elemu/                       # 参考OSS: ECHONET Liteエミュレータ
│   ├── README.md               # elemuの説明
│   ├── lib/                    # ライブラリファイル
│   └── ...
└── echonet-simulator/          # 新規開発: ECHONET Liteシミュレータ
    ├── README.md               # シミュレータの説明
    ├── src/                    # TypeScriptソースコード
    ├── config/                 # 設定ファイル
    ├── Dockerfile              # Docker設定
    └── ...
```

## シミュレータの種類

### 1. elemu (参考OSS)
- **目的**: ECHONET Liteプロトコルの参考実装
- **技術**: Node.js
- **機能**: EV充電器クラス（0x02A1, 0x027E）の実装
- **用途**: 新規シミュレータ開発の参考資料

### 2. echonet-simulator (新規開発)
- **目的**: OCPP Gatewayのテスト用ECHONET Liteシミュレータ
- **技術**: TypeScript + Node.js
- **機能**: 
  - EV充電器・充放電器のシミュレーション
  - 複数インスタンス対応
  - シミュレーションモード（動的プロパティ変更）
  - REST API・CLI（予定）
  - Docker対応
- **用途**: OCPP Gatewayの統合テスト環境

### 3. oss-csms (OSSベースCSMS)
- **目的**: OCPP Gatewayのテスト用CSMSシミュレータ
- **技術**: Python + OSSライブラリ
- **機能**:
  - python-ocppベースCSMS
  - EVerest/ocpp-csms統合
  - DockerベースCSMS
  - 充電器エミュレーター
- **用途**: OCPP GatewayのCSMS統合テスト

## 使用方法

### echonet-simulatorの起動

```bash
cd simulators/echonet-simulator
npm install
npm run build
npm start
```

### Dockerでの起動

```bash
cd simulators/echonet-simulator
docker build -t echonet-simulator .
docker run -p 3610:3610 echonet-simulator
```

### oss-csmsの起動

#### python-ocppベースCSMS
```bash
cd simulators/oss-csms/python-ocpp
./setup.sh
source venv/bin/activate
python csms_server.py
```

#### EVerest/ocpp-csms
```bash
cd simulators/oss-csms/everest
./setup.sh
source venv/bin/activate
cd ocpp-csms
python main.py
```

#### DockerベースCSMS
```bash
cd simulators/oss-csms/docker
docker-compose up -d
```

## 開発方針

1. **分離**: メインアプリケーションとシミュレータを明確に分離
2. **再利用**: elemuの実装を参考にしつつ、新規要件に対応
3. **拡張性**: 新しいデバイスクラスやプロパティの追加が容易
4. **テスト**: OCPP Gatewayとの統合テストに最適化

## 今後の予定

- [x] ECHONET Liteシミュレータの実装
- [x] OSSベースCSMSの統合
- [ ] EV充放電器クラス（0x027E）の実装
- [ ] REST APIの実装
- [ ] CLIの実装
- [ ] 設定ファイルのホットリロード
- [ ] ログ機能の強化
- [ ] OCPP Gatewayとの統合テスト
- [ ] 自動テストスクリプトの作成 