# 🎨 OCPP CSMS 通信可視化ガイド

## 📖 概要

OCPP CSMSシミュレータに強力な可視化機能を追加しました。リアルタイムで通信状況を監視し、デバッグを効率化できます。

## 🚀 起動方法

### 1. 依存関係のインストール

```bash
cd simulators/oss-csms/python-ocpp
source venv/bin/activate
pip install colorama flask flask-socketio
```

### 2. CSMSシミュレータの起動

```bash
python simple_csms.py
```

起動すると以下が表示されます：
- 🌈 **カラフルなコンソール出力**
- 🌐 **WebSocket サーバー**: `ws://0.0.0.0:9000`
- 📊 **Web ダッシュボード**: `http://localhost:8081`

## 🎨 可視化機能

### 1. コンソール可視化

#### ✨ 機能
- **カラフルなログ出力** - メッセージタイプ別の色分け
- **リアルタイム接続状況** - 充電ステーション接続/切断
- **詳細メッセージ表示** - 送受信メッセージの構造化表示
- **統計情報** - アクティブ接続数、総メッセージ数、稼働時間

#### 🎯 表示例
```
================================================================================
  OCPP 2.0.1 CSMS Simulator - Communication Monitor  
================================================================================
🚀 Server Status: RUNNING
📡 WebSocket Port: 9000
🔗 Protocol: OCPP 2.0.1
⏰ Started: 2025-07-31 22:38:04
================================================================================

✅ CSMS Server successfully started!
🌐 OCPP WebSocket: ws://0.0.0.0:9000
📊 Web Dashboard: http://localhost:8081
⏳ Waiting for OCPP charging stations to connect...
============================================================

[22:38:34] 🔌 CP001 CONNECTED

[22:38:34] 📥 RECEIVED from/to CP001
└─ Message Type: BootNotification
└─ Payload:
   ├─ chargingStation:
   │  └─ serialNumber: CP001
   │  └─ model: Test CP
   │  └─ vendorName: TestVendor
   └─ reason: PowerUp
💬 Total Messages: 1
------------------------------------------------------------

[22:38:34] 📤 SENT from/to CP001
└─ Message Type: Response
└─ Payload:
   ├─ status: Accepted
   ├─ currentTime: 2025-07-31T13:38:34.483553
   └─ interval: 300
💬 Total Messages: 2
------------------------------------------------------------
```

### 2. Web ダッシュボード

#### 🌐 アクセス方法
ブラウザで `http://localhost:8081` にアクセス

#### ✨ 機能
- **リアルタイム統計** - サーバー状態、接続数、メッセージ数、稼働時間
- **アクティブ接続監視** - 接続中の充電ステーション一覧
- **ライブメッセージ表示** - 送受信メッセージのリアルタイム表示
- **レスポンシブデザイン** - PC・タブレット・スマホ対応
- **自動更新** - データの自動リフレッシュ

#### 📊 画面構成
```
┌─────────────────────────────────────────────────────────────┐
│                  OCPP CSMS Dashboard                        │
├─────────────────────────────────────────────────────────────┤
│ [Server Status]  [Active Connections]  [Total Messages]    │
│     ONLINE             2                    47              │
│                                                             │
│ ┌─────────────────┐  ┌─────────────────────────────────────┐│
│ │ Active          │  │ Live Messages                       ││
│ │ Connections     │  │                                     ││
│ │                 │  │ 📥 BootNotification from CP001     ││
│ │ 🔌 CP001        │  │ 📤 Response to CP001               ││
│ │ 🔌 CP002        │  │ 📥 Heartbeat from CP001            ││
│ │                 │  │ 📤 Response to CP001               ││
│ └─────────────────┘  └─────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

## 🔧 使用例

### 1. 基本的な通信テスト

```bash
# ターミナル1: CSMSを起動
python simple_csms.py

# ターミナル2: テスト用充電ポイントクライアントを実行
python test_charge_point.py

# ブラウザ: Webダッシュボードを開く
# http://localhost:8081
```

### 2. 複数接続のテスト

```bash
# 複数のクライアントを同時実行
python test_charge_point.py &
python test_charge_point.py &
python test_charge_point.py &
```

### 3. 長時間監視

```bash
# バックグラウンドで実行
nohup python simple_csms.py > csms_monitor.log 2>&1 &

# ログ監視
tail -f csms_monitor.log
```

## 🎯 デバッグ支援機能

### 1. メッセージトレース
- 全メッセージの送受信履歴
- タイムスタンプ付きログ
- ペイロード構造の可視化

### 2. 接続状態監視
- リアルタイム接続状況
- 接続時間の記録
- 切断検知とアラート

### 3. 統計情報
- メッセージ処理数
- アクティブ接続数
- サーバー稼働時間

## 🛠️ カスタマイズ

### 1. ログレベル調整

```python
# simple_csms.py の設定を変更
logging.basicConfig(level=logging.DEBUG)  # より詳細なログ
```

### 2. Webダッシュボードポート変更

```python
# simple_csms.py の初期化部分を変更
self.web_dashboard = start_dashboard(port=8082)  # ポート変更
```

### 3. 統計更新間隔調整

```python
# simple_csms.py の statistics_monitor関数を変更
await asyncio.sleep(10)  # 10秒間隔に変更
```

## 🎨 色分けガイド

### コンソール出力
- 🟢 **緑**: 成功・接続・正常状態
- 🔵 **青**: 受信メッセージ・情報
- 🟣 **紫**: 送信メッセージ・応答
- 🟡 **黄**: 警告・待機状態
- 🔴 **赤**: エラー・切断・異常状態

### Webダッシュボード
- **緑**: アクティブ接続・正常状態
- **青**: 受信メッセージ
- **紫**: 送信メッセージ
- **オレンジ**: 統計情報
- **赤**: エラー・異常状態

## 🚀 パフォーマンス

- **低遅延**: リアルタイム表示
- **軽量**: 最小限のリソース使用
- **スケーラブル**: 複数接続対応
- **安定性**: 長時間稼働対応

## 📞 サポート

問題が発生した場合：

1. **ポート確認**: `ss -tuln | grep -E "(9000|8081)"`
2. **プロセス確認**: `ps aux | grep python`
3. **ログ確認**: CSMSコンソール出力をチェック
4. **依存関係確認**: `pip list | grep -E "(flask|colorama)"`

---

**🎉 これでOCCP通信の可視化が完璧に動作します！**

リアルタイムで美しく表示される通信ログとWebダッシュボードで、デバッグ効率が大幅に向上します。