# OCPP Gateway ソースコード概要

## 1. プロジェクト全体の構造

OCPP 2.0.1 対応ゲートウェイ・ミドルウェアは、充電ステーション管理システム（CSMS）と充電器群の間でプロトコル変換を行うシステムです。

### 主要なディレクトリ構造
```
src/
├── main.cpp                    # エントリーポイント
├── common/                     # 共通機能
├── api/                        # API機能
├── device/                     # デバイス通信
├── mapping/                    # マッピング機能
└── ocpp/                      # OCPP通信機能
```

## 2. 各コンポーネントの役割と処理

### 2.1 エントリーポイント (`src/main.cpp`)

**役割**: アプリケーションの起動とコマンドライン引数の処理

**処理の概要**:
- コマンドラインオプションの解析（設定ファイルパス、デーモンモード、CLIモード等）
- `Application`クラスの初期化と実行
- エラーハンドリングと終了コードの管理

**関連コンポーネント**: `Application`クラス

### 2.2 共通機能 (`src/common/`)

#### `Application` (`application.cpp`)
**役割**: システム全体のライフサイクル管理

**処理の概要**:
- コンポーネントの初期化・終了
- シグナルハンドリング
- デーモンモード/CLIモードの実行制御
- 設定の再読み込み

**関連コンポーネント**: 全コンポーネント

#### `ConfigManager` (`config_manager.cpp`)
**役割**: 設定ファイルの管理

**処理の概要**:
- YAML設定ファイルの読み込み・解析
- 設定の動的更新
- 設定値の検証

**関連コンポーネント**: 全コンポーネント

#### `Logger` (`logger.cpp`)
**役割**: ログ出力の管理

**処理の概要**:
- ログレベルの制御
- ファイル・コンソール出力
- ログローテーション

#### `MetricsCollector` (`metrics_collector.cpp`)
**役割**: メトリクス収集

**処理の概要**:
- システムメトリクスの収集
- Prometheus形式での出力
- パフォーマンス監視

#### `PrometheusExporter` (`prometheus_exporter.cpp`)
**役割**: Prometheusメトリクス出力

**処理の概要**:
- HTTPエンドポイントでのメトリクス提供
- Prometheus形式でのデータ出力
- メトリクス収集の統合

#### `TlsManager` (`tls_manager.cpp`)
**役割**: TLS証明書管理

**処理の概要**:
- 証明書の読み込み・検証
- クライアント証明書管理
- セキュリティ設定

#### `RbacManager` (`rbac_manager.cpp`)
**役割**: ロールベースアクセス制御

**処理の概要**:
- ユーザー認証・認可
- 権限管理
- セキュリティポリシー適用

#### `SystemConfig` (`system_config.cpp`)
**役割**: システム設定管理

**処理の概要**:
- システム全体の設定管理
- 環境変数の処理
- デフォルト設定の提供

#### `CsmsConfig` (`csms_config.cpp`)
**役割**: CSMS接続設定管理

**処理の概要**:
- CSMS接続設定の管理
- WebSocket URL設定
- 接続パラメータ管理

#### `DeviceConfig` (`device_config.cpp`)
**役割**: デバイス設定管理

**処理の概要**:
- デバイス設定の管理
- プロトコル別設定
- デバイステンプレート管理

#### `FileWatcher` (`file_watcher.cpp`)
**役割**: ファイル変更監視

**処理の概要**:
- 設定ファイルの変更監視
- 自動設定再読み込み
- ファイルシステムイベント処理

#### `I18nManager` (`i18n_manager.cpp`)
**役割**: 国際化対応

**処理の概要**:
- 多言語対応
- メッセージリソース管理
- ロケール設定

#### `LanguageManager` (`language_manager.cpp`)
**役割**: 言語管理

**処理の概要**:
- 言語設定の管理
- 翻訳リソースの読み込み
- 言語切り替え機能

#### `ServiceManager` (`service_manager.cpp`)
**役割**: サービス管理

**処理の概要**:
- システムサービスの管理
- サービスの開始・停止
- 依存関係の管理

#### `ErrorHandling` (`error_handling.cpp`)
**役割**: エラーハンドリング

**処理の概要**:
- エラー処理の統一
- エラーログ出力
- エラー回復機能

### 2.3 API機能 (`src/api/`)

#### `AdminApi` (`admin_api.cpp`)
**役割**: RESTful APIサーバー

**処理の概要**:
- HTTP/HTTPSサーバーの提供
- 設定管理API
- デバイス管理API
- 監視機能API

**関連コンポーネント**: `ConfigManager`, `DeviceAdapter`

#### `WebUI` (`web_ui.cpp`)
**役割**: Web管理インターフェース

**処理の概要**:
- Web UIサーバーの提供
- リアルタイム監視画面
- 設定変更インターフェース

#### `CliManager` (`cli_manager.cpp`)
**役割**: コマンドライン管理インターフェース

**処理の概要**:
- 対話型CLI
- バッチコマンド実行
- システム管理コマンド

### 2.4 デバイス通信 (`src/device/`)

#### `DeviceAdapter` (`device_adapter.cpp`)
**役割**: デバイス通信の基底クラス

**処理の概要**:
- デバイス登録・削除
- レジスタ読み書き
- デバイス状態監視
- 非同期通信

**関連コンポーネント**: `EchonetLiteAdapter`, `ModbusRtuAdapter`

#### `EchonetLiteAdapter` (`echonet_lite_adapter.cpp`)
**役割**: ECHONET Lite通信

**処理の概要**:
- UDP通信によるデバイス発見
- EPC（Echonet Property Code）の読み書き
- マルチキャスト通信
- デバイス状態監視

#### `ModbusRtuAdapter` (`modbus_rtu_adapter.cpp`)
**役割**: Modbus RTU通信

**処理の概要**:
- シリアル通信
- レジスタ読み書き
- 通信エラー処理
- タイムアウト管理

#### `ModbusTcpAdapter` (`modbus_tcp_adapter.cpp`)
**役割**: Modbus TCP通信

**処理の概要**:
- TCP通信
- レジスタ読み書き
- 接続管理

### 2.5 マッピング機能 (`src/mapping/`)

#### `MappingEngine` (`mapping_engine.cpp`)
**役割**: プロトコル変換エンジン

**処理の概要**:
- OCPP変数とデバイス値の変換
- マッピング設定の管理
- デバイスアダプターの統合

**関連コンポーネント**: `DeviceAdapter`, `OcppClientManager`

### 2.6 OCPP通信 (`src/ocpp/`)

#### `OcppClientManager` (`ocpp_client_manager.cpp`)
**役割**: OCPP WebSocket接続管理

**処理の概要**:
- CSMSとのWebSocket接続
- メッセージの送受信
- 接続状態管理
- ハートビート管理

**関連コンポーネント**: `WebSocketClient`, `OcppMessageProcessor`

#### `WebSocketClient` (`websocket_client.cpp`)
**役割**: WebSocket通信

**処理の概要**:
- TLS/WebSocket接続
- メッセージ送受信
- 接続エラー処理

#### `OcppMessageProcessor` (`ocpp_message_processor.cpp`)
**役割**: OCPPメッセージ処理

**処理の概要**:
- JSONメッセージの解析
- メッセージルーティング
- レスポンス生成

#### `OcppMessageHandlers` (`ocpp_message_handlers.cpp`)
**役割**: OCPPメッセージハンドラー

**処理の概要**:
- 各種OCPPメッセージの処理
- 充電ステーション状態管理
- トランザクション管理

#### `EvseStateMachine` (`evse_state_machine.cpp`)
**役割**: 充電ステーション状態管理

**処理の概要**:
- 充電ステーションの状態遷移
- イベント処理
- 状態通知

#### `BootNotification` (`boot_notification.cpp`)
**役割**: 起動通知処理

**処理の概要**:
- 充電ステーションの起動通知
- モデル・ベンダー情報の送信

#### `TransactionEvent` (`transaction_event.cpp`)
**役割**: トランザクションイベント処理

**処理の概要**:
- 充電トランザクションの開始・終了
- メーター値の送信
- トランザクション状態管理

#### `StatusNotification` (`status_notification.cpp`)
**役割**: 状態通知処理

**処理の概要**:
- 充電ステーションの状態変更通知
- エラー状態の通知

#### `Heartbeat` (`heartbeat.cpp`)
**役割**: ハートビート処理

**処理の概要**:
- 定期的なハートビート送信
- 接続状態の維持

#### `VariableTranslator` (`variable_translator.cpp`)
**役割**: OCPP変数変換

**処理の概要**:
- OCPP変数とデバイス値の変換
- データ型変換
- 単位変換

#### `MappingConfig` (`mapping_config.cpp`)
**役割**: マッピング設定管理

**処理の概要**:
- マッピング設定の読み込み
- デバイステンプレート管理
- 設定の動的更新

#### `Message` (`message.cpp`)
**役割**: OCPPメッセージ構造体

**処理の概要**:
- OCPPメッセージの定義
- メッセージ形式の管理
- メッセージ検証

#### `MessageFactory` (`message_factory.cpp`)
**役割**: OCPPメッセージ生成

**処理の概要**:
- OCPPメッセージの生成
- メッセージテンプレート管理
- メッセージ構築

## 3. コンポーネント間の関連

### 3.1 データフロー
```
[CSMS] ←→ [OcppClientManager] ←→ [MappingEngine] ←→ [DeviceAdapter] ←→ [充電器]
                ↓                        ↓                    ↓
        [WebSocketClient]        [VariableTranslator]    [EchonetLiteAdapter]
                ↓                        ↓                    ↓
        [OcppMessageProcessor]    [MappingConfig]        [ModbusRtuAdapter]
```

### 3.2 主要な依存関係
- `Application` → 全コンポーネント
- `OcppClientManager` → `WebSocketClient`, `OcppMessageProcessor`
- `MappingEngine` → `DeviceAdapter`, `VariableTranslator`
- `DeviceAdapter` → `EchonetLiteAdapter`, `ModbusRtuAdapter`
- `AdminApi` → `ConfigManager`, `DeviceAdapter`

### 3.3 設定管理
- `ConfigManager` が全コンポーネントの設定を管理
- YAMLファイルによる設定
- 動的設定更新機能

## 4. ファイルサイズと複雑度

### 4.1 大きなファイル（1000行以上）
- `device_config.cpp` (1401行) - デバイス設定管理
- `echonet_lite_adapter.cpp` (1219行) - ECHONET Lite通信
- `modbus_rtu_adapter.cpp` (1356行) - Modbus RTU通信
- `web_ui.cpp` (798行) - Web管理インターフェース
- `cli_manager.cpp` (722行) - コマンドライン管理
- `mapping_config.cpp` (1327行) - マッピング設定管理

### 4.2 中程度のファイル（300-700行）
- `application.cpp` (374行) - アプリケーション管理
- `language_manager.cpp` (373行) - 言語管理
- `metrics_collector.cpp` (482行) - メトリクス収集
- `rbac_manager.cpp` (476行) - アクセス制御
- `system_config.cpp` (400行) - システム設定
- `evse_state_machine.cpp` (642行) - 充電ステーション状態管理
- `transaction_event.cpp` (461行) - トランザクション処理
- `variable_translator.cpp` (520行) - 変数変換

### 4.3 小さなファイル（100行以下）
- `mapping_engine.cpp` (66行) - マッピングエンジン
- `heartbeat.cpp` (58行) - ハートビート処理

## 5. 技術スタック

### 5.1 主要ライブラリ
- **Boost.Asio** - 非同期I/O
- **websocketpp** - WebSocket通信
- **libmodbus** - Modbus通信
- **YAML-CPP** - YAML設定ファイル解析
- **nlohmann/json** - JSON処理
- **spdlog** - ログ出力

### 5.2 通信プロトコル
- **OCPP 2.0.1** - 充電ステーション管理プロトコル
- **ECHONET Lite** - 家庭・ビル用ネットワークシステム
- **Modbus RTU/TCP** - 産業用通信プロトコル
- **HTTP/HTTPS** - Web API
- **WebSocket** - リアルタイム通信

## 6. セキュリティ機能

### 6.1 認証・認可
- `RbacManager` - ロールベースアクセス制御
- TLS証明書管理
- ユーザー認証

### 6.2 通信セキュリティ
- TLS/SSL暗号化
- WebSocket over TLS
- 証明書検証

### 6.3 エラーハンドリング
- 統一エラー処理
- エラーログ出力
- エラー回復機能

## 7. 監視・運用機能

### 7.1 メトリクス収集
- Prometheus形式でのメトリクス出力
- システムパフォーマンス監視
- カスタムメトリクス

### 7.2 ログ管理
- 構造化ログ出力
- ログローテーション
- ログレベル制御

### 7.3 管理インターフェース
- RESTful API
- Web管理画面
- コマンドライン管理

このアーキテクチャにより、OCPP 2.0.1プロトコルと各種充電器プロトコル（ECHONET Lite、Modbus）の間でシームレスな変換が実現されています。 