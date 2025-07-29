# OSS-based CSMS Simulators

OCPP Gatewayのテスト用に、既存のOSSを活用したCSMSシミュレータを提供します。

## 推奨OSS

### 1. python-ocpp (The Mobility House)
- **ライセンス**: MIT
- **特徴**: 
  - OCPP 1.6/2.0.1対応
  - WebSocketサーバーを数十行で構築可能
  - 充電器エミュレーターとCSMSの両方に対応
  - 豊富なドキュメントとサンプル

### 2. EVerest/ocpp-csms
- **ライセンス**: MIT
- **特徴**:
  - python-ocppベース
  - 「友好的に応答する」シンプルなCSMS
  - ISO15118証明書インストール対応
  - 数行のコマンドで起動

### 3. ocpp-cs (C++ CSMS)
- **ライセンス**: MIT
- **特徴**:
  - OCPP 1.5/1.6対応
  - Docker対応
  - WebUI付き
  - 充電器エミュレーター機能も含む

## セットアップ

### python-ocpp ベースCSMS

```bash
# 仮想環境作成
python3 -m venv venv
source venv/bin/activate

# インストール
pip install ocpp

# 簡易CSMS起動
python simple_csms.py
```

### EVerest/ocpp-csms

```bash
# クローン
git clone https://github.com/EVerest/ocpp-csms.git
cd ocpp-csms

# 依存関係インストール
pip install -r requirements.txt

# 起動
python main.py
```

### ocpp-cs (Docker)

```bash
# Dockerイメージ取得
docker pull ocpp-cs/csms

# 起動
docker run -p 9000:9000 ocpp-cs/csms
```

## 使用方法

### OCPP Gatewayとの接続

1. **CSMS起動**
   ```bash
   cd simulators/oss-csms/python-ocpp
   python csms_server.py
   ```

2. **OCPP Gateway設定**
   ```yaml
   # config/system.yaml
   csms:
     url: "ws://localhost:9000/CSMS001"
     protocol: "ocpp2.0.1"
   ```

3. **テスト実行**
   - 充電器からの接続テスト
   - トランザクション開始/終了テスト
   - メーター値送信テスト

## ディレクトリ構成

```
simulators/oss-csms/
├── README.md                    # このファイル
├── python-ocpp/                 # python-ocppベースCSMS
│   ├── csms_server.py          # 簡易CSMSサーバー
│   ├── charge_point.py         # 充電器エミュレーター
│   └── requirements.txt        # 依存関係
├── everest/                     # EVerest/ocpp-csms
│   ├── setup.sh               # セットアップスクリプト
│   └── config.yaml            # 設定ファイル
└── docker/                      # DockerベースCSMS
    ├── docker-compose.yml      # Docker Compose設定
    └── config/                 # 設定ファイル
```

## メリット

1. **開発効率**: 既存の実装を活用
2. **信頼性**: コミュニティでテスト済み
3. **機能豊富**: 本格的なOCPP機能
4. **保守性**: 継続的なアップデート
5. **コンプライアンス**: 規格準拠の実装

## 今後の予定

- [ ] python-ocppベースCSMSの実装
- [ ] EVerest/ocpp-csmsの統合
- [ ] Docker環境の構築
- [ ] OCPP Gatewayとの統合テスト
- [ ] 自動テストスクリプトの作成 