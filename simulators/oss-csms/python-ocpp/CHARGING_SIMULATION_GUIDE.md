# 🔋 OCPP充電シミュレーション使用ガイド

## 📖 概要

OCPP 2.0.1準拠の充電シミュレーション機能を実装しました。実際の充電ステーションの動作を模擬し、リアルタイムで充電データを生成します。

## 🚀 起動方法

### 1. 依存関係のインストール

```bash
cd simulators/oss-csms/python-ocpp
source venv/bin/activate
pip install websockets asyncio
```

### 2. シミュレータ環境の起動

#### ECHONET Liteシミュレータ
```bash
cd /home/ubuntu/ocpp-gw/simulators/echonet-simulator
nohup node dist/index.js > echonet.log 2>&1 &
```

#### CSMSシミュレータ（可視化機能付き）
```bash
cd /home/ubuntu/ocpp-gw/simulators/oss-csms/python-ocpp
source venv/bin/activate
nohup python simple_csms.py > csms.log 2>&1 &
```

#### 起動確認
```bash
ss -tuln | grep -E "(3610|9000|8081)"
```

期待される出力：
```
udp   UNCONN  0       0               0.0.0.0:3610         0.0.0.0:*            
tcp   LISTEN  0       100             0.0.0.0:9000         0.0.0.0:*            
tcp   LISTEN  0       128             0.0.0.0:8081         0.0.0.0:*            
```

## 🔋 充電シミュレーション実行

### 1. 基本的な充電セッション

```bash
python charging_simulator.py
```

#### 実行例
```
🚀 Starting OCPP Charging Station Simulator
📡 Station ID: CP001
🌐 CSMS URL: ws://localhost:9000/CP001
🔋 Sessions: 1
🔌 Connecting to CSMS: ws://localhost:9000/CP001
✅ Connected to CSMS
🚗 Starting charging session simulation...
⚡ Starting charging simulation (30 seconds)...
🔋 Charging progress: 3.3% - 0.212 kWh
🔋 Charging progress: 20.0% - 1.475 kWh
🔋 Charging progress: 36.7% - 3.168 kWh
🔋 Charging progress: 53.3% - 4.299 kWh
🔋 Charging progress: 70.0% - 5.353 kWh
🔋 Charging progress: 86.7% - 6.756 kWh
✅ Charging session completed! Total energy: 8.114 kWh
```

### 2. 複数セッションの実行

```bash
# 3回の充電セッションを連続実行
python charging_simulator.py --sessions 3

# 5回の充電セッションを連続実行
python charging_simulator.py --sessions 5
```

### 3. カスタム設定での実行

```bash
# 異なる充電ステーションIDで実行
python charging_simulator.py --station-id CP002

# カスタムCSMS URLで実行
python charging_simulator.py --csms-url ws://localhost:9000/CP002

# 複数セッション + カスタム設定
python charging_simulator.py --station-id CP003 --sessions 3
```

## 📊 充電データシミュレーション

### リアルタイム生成データ

#### エネルギー消費量
- **範囲**: 0.1-0.5 kWh/秒
- **累積**: 30秒間で約8-9 kWh
- **精度**: 小数点以下3桁

#### 充電電力
- **範囲**: 5-22 kW
- **変動**: ランダム変動でリアルな動作を模擬
- **単位**: kW

#### 電圧・電流・周波数
- **電圧**: 220-240V（ランダム変動）
- **電流**: 電力/電圧で動的計算
- **周波数**: 49.8-50.2Hz（ランダム変動）

### OCPPメッセージフロー

```
1. BootNotification     → 充電ステーション起動通知
2. StatusNotification   → 接続状態変更通知
3. Authorize           → 認証要求
4. TransactionEvent    → 取引開始イベント
5. MeterValues         → 計測値送信（30秒間1秒間隔）
6. TransactionEvent    → 取引終了イベント
7. StatusNotification  → 利用可能状態通知
```

## 🎨 可視化機能

### 1. コンソール可視化

#### 充電進捗表示
```
🔋 Charging progress: 20.0% - 1.475 kWh
🔋 Charging progress: 36.7% - 3.168 kWh
🔋 Charging progress: 53.3% - 4.299 kWh
```

#### メーター値表示
```
Sending MeterValues: 0.212 kWh, 12.45 kW
Sending MeterValues: 0.625 kWh, 15.56 kW
Sending MeterValues: 0.920 kWh, 7.39 kW
```

### 2. Webダッシュボード

#### アクセス方法
ブラウザで `http://localhost:8081` にアクセス

#### 表示内容
- **リアルタイム統計**: サーバー状態、接続数、メッセージ数
- **アクティブ接続**: 充電ステーション接続状況
- **ライブメッセージ**: 送受信メッセージのリアルタイム表示

## 🔧 高度な使用方法

### 1. 長時間シミュレーション

```bash
# 10回の充電セッションを実行
python charging_simulator.py --sessions 10

# バックグラウンドで実行
nohup python charging_simulator.py --sessions 5 > charging.log 2>&1 &
```

### 2. 複数充電ステーション

```bash
# ターミナル1: CP001で充電
python charging_simulator.py --station-id CP001 &

# ターミナル2: CP002で充電
python charging_simulator.py --station-id CP002 &

# ターミナル3: CP003で充電
python charging_simulator.py --station-id CP003 &
```

### 3. パフォーマンステスト

```bash
# 同時に5つの充電ステーションで充電
for i in {1..5}; do
    python charging_simulator.py --station-id CP00$i &
done
```

## 📈 パフォーマンス仕様

### 通信性能
- **メッセージ処理**: 30秒間で約90メッセージ
- **データ精度**: 小数点以下3桁のエネルギー消費量
- **リアルタイム性**: 1秒間隔での計測値更新
- **安定性**: 複数セッション連続実行成功

### 可視化性能
- **コンソール表示**: 即座に反映
- **Webダッシュボード**: リアルタイム更新
- **メモリ使用量**: 軽量（最小限のリソース）
- **スケーラビリティ**: 複数接続対応

## 🛠️ トラブルシューティング

### 1. 接続エラー

```bash
# ポート確認
ss -tuln | grep -E "(9000|8081)"

# プロセス確認
ps aux | grep python

# ログ確認
tail -f csms.log
```

### 2. 充電シミュレーションエラー

```bash
# 依存関係確認
pip list | grep websockets

# 手動テスト
python -c "import websockets; print('OK')"
```

### 3. Webダッシュボードエラー

```bash
# ポート確認
curl http://localhost:8081/api/status

# ログ確認
tail -f csms.log | grep -i error
```

## 🎯 使用例

### 1. 基本的なテスト

```bash
# 1. シミュレータ起動
cd /home/ubuntu/ocpp-gw/simulators/oss-csms/python-ocpp
source venv/bin/activate
nohup python simple_csms.py > csms.log 2>&1 &

# 2. 充電シミュレーション実行
python charging_simulator.py

# 3. Webダッシュボード確認
# ブラウザで http://localhost:8081 にアクセス
```

### 2. 複数セッションテスト

```bash
# 3回の充電セッションを実行
python charging_simulator.py --sessions 3

# 結果確認
echo "充電セッション完了！"
```

### 3. 長時間テスト

```bash
# 10回の充電セッションをバックグラウンドで実行
nohup python charging_simulator.py --sessions 10 > long_test.log 2>&1 &

# 進捗確認
tail -f long_test.log
```

## 🎉 特徴

### ✅ 実装済み機能
1. **完全な充電セッションシミュレーション**
2. **リアルタイム充電データ生成**
3. **美しい可視化機能**
4. **Webダッシュボード統合**
5. **複数セッション対応**
6. **安定した通信性能**

### 🚀 実用性
- 🚗 **実際の充電ステーション動作を模擬**
- 📊 **詳細な充電データの可視化**
- 🔧 **デバッグ・テスト環境として活用可能**
- 📈 **パフォーマンス監視機能**

---

**🎉 これで充電シミュレーションが完璧に動作します！**

実際の充電ステーションの動作を模擬した完全なシミュレーション環境で、OCPPゲートウェイの開発・テストに活用できます。 