## OCPP 2.0.1 対応ゲートウェイ・ミドルウェア

### 要件定義書（ドラフト）

---

### 1. 目的

既存の ECHONET Lite（以下 **EL**）または Modbus（RTU/TCP）対応充電器を **OCPP 2.0.1** ネットワークへ容易に統合し、最大 100 台の充電器を一括管理できるゲートウェイ・ミドルウェア（以降 **本ゲートウェイ**）を C++ で開発する。

---

### 2. 適用範囲

* 本ゲートウェイのソフトウェア・アーキテクチャ、機能／非機能要件、運用・保守方針
* ハードウェア（Linux 組み込み PC 等）および通信インタフェース要件
* 充電器側ファームウェア変更は対象外（通信マッピング設定のみ対応）

---

### 3. 用語・略語

| 用語             | 定義                                                |
| -------------- | ------------------------------------------------- |
| **CSMS**       | Central System Management System（OCPP サーバ、バックエンド） |
| **EVSE**       | Electric Vehicle Supply Equipment（充電器またはそのコネクタ単位） |
| **OCPP**       | Open Charge Point Protocol。ここでは 2.0.1 を指す         |
| **EL**         | ECHONET Lite（UDP/IP ベースのスマートホーム／IoT プロトコル）        |
| **Modbus RTU** | RS‑485 物理層を用いるバイナリ通信プロトコル                         |
| **Modbus TCP** | TCP/IP 上で動作する Modbus                              |

---

### 4. 参照規格・ドキュメント

1. OCPP 2.0.1 Specification（Open Charge Alliance）
2. OCPP 2.0.1 Security White Paper
3. Modbus Application Protocol v1.1b3
4. ECHONET Lite 規格書（Release J）
5. TLS 1.2 / TLS 1.3 RFC

---

### 5. システム構成概要

```
[CSMS] ⇄(TLS/WebSocket)⇄ [本ゲートウェイ (Linux/C++)] ⇄
           ├─ UDP/IP (EL) ─▶ 充電器A…N
           └─ Modbus(TCP/RTU) ─▶ 充電器A…N
```

* **OCPP クライアントモジュール**： チャージポイントを最大 100 件生成し、それぞれ CSMS と恒常的に WebSocket 接続
* **デバイス・アダプタ**： EL 用 UDP ハンドラ、Modbus TCP/RTU 用ハンドラ
* **マッピングエンジン**： 外部定義ファイル（JSON/YAML）で OCPP 変数 ↔ 充電器レジスタ／EPC を結合
* **管理 UI／CLI**： デバイス登録・設定変更・状態監視・ログ出力

---

### 6. 機能要件

| 区分              | 要件 ID     | 要件内容                                                                                                  |
| --------------- | --------- | ----------------------------------------------------------------------------------------------------- |
| **6.1 OCPP 対応** | F‑OCPP‑01 | **Core Profile 完全実装**（BootNotification, Heartbeat, StatusNotification, TransactionEvent, Authorize 等） |
|                 | F‑OCPP‑02 | **Remote 操作**：Remote Start / Stop Transaction, UnlockConnector, TriggerMessage                        |
|                 | F‑OCPP‑03 | **Metering**：MeterValues 送信（取引中 1 分間隔／取引終了時）                                                          |
|                 | F‑OCPP‑04 | **Smart Charging（基本）**：SetChargingProfile による電流上限設定                                                   |
|                 | F‑OCPP‑05 | **優先度中**：LocalAuthList, Reservation, FirmwareUpdate, Get/SetVariables など（Phase 2 以降）                  |
| **6.2 プロトコル変換** | F‑MAP‑01  | 外部定義ファイルで OCPP 変数 ↔ Modbus レジスタ／EPC を動的に設定可能                                                          |
|                 | F‑MAP‑02  | デバイス別テンプレート機能：モデル追加時はテンプレートを流用し差分のみ編集                                                                 |
|                 | F‑MAP‑03  | 変換ロジックでスケーリング・単位変換（例：0.1 kWh → Wh）をサポート                                                               |
| **6.3 デバイス通信**  | F‑DEV‑01  | EL：UDP／IPv4 マルチキャスト探索＋ユニキャスト制御。再送・タイムアウト管理                                                            |
|                 | F‑DEV‑02  | Modbus RTU：RS‑485 最大 25 台／バス。ポーリング周期・CRC エラー再送                                                        |
|                 | F‑DEV‑03  | Modbus TCP：非同期ソケット／コネクションプールで並列処理                                                                     |
| **6.4 同時接続数**   | F‑SCAL‑01 | 充電器最大 100 台＋ OCPP WebSocket 100 本を同時維持                                                                |
| **6.5 セキュリティ**  | F‑SEC‑01  | TLS 1.2 以上、サーバ証明書検証必須。クライアント証明書（双方向認証）はオプション実装                                                        |
|                 | F‑SEC‑02  | 構成ファイル・証明書は暗号化領域に保管。管理 UI は RBAC＋HTTPS                                                                |
| **6.6 ロギング／監視** | F‑MON‑01  | デバイス通信・OCPP メッセージ／エラーをレベル別にログ出力（rotate, 圧縮）                                                           |
|                 | F‑MON‑02  | REST または Prometheus exporter で死活・統計情報を外部監視へ提供                                                         |
| **6.7 障害／再接続**  | F‑RES‑01  | CSMS 切断時はオフライン取引をキューイング、再接続後自動送信                                                                      |
|                 | F‑RES‑02  | デバイス無応答時は一定回数リトライ後 Faulted を CSMS に通知                                                                 |
| **6.8 更新**      | F‑UPD‑01  | Debian‑based パッケージ（.deb）または OTA 更新スクリプトを提供                                                            |
| **6.9 多言語**     | F‑I18N‑01 | ログ・UI は日本語／英語切替可（初期は英語ベース、日本語リソース同梱）                                                                  |

---

### 7. 非機能要件

| 区分          | 要件 ID      | 指標／基準                                            |
| ----------- | ---------- | ------------------------------------------------ |
| **7.1 性能**  | NF‑PERF‑01 | WebSocket 往復遅延：通常 < 500 ms                       |
|             | NF‑PERF‑02 | 100 台接続時、CPU 使用率 < 60 %（4 Core ARMv8 1.6 GHz 相当） |
| **7.2 可用性** | NF‑AVL‑01  | 365 日稼働、MTBF ≥ 5,000 h／台                         |
| **7.3 拡張性** | NF‑EXT‑01  | デバイスアダプタをプラグイン追加可能（将来 CAN や OCPP 2.1 等）          |
| **7.4 保守性** | NF‑MNT‑01  | コンフィグ変更はサービス再起動なしで反映（HUP シグナル等）                  |
| **7.5 移植性** | NF‑PORT‑01 | Debian/Ubuntu 22.04, Yocto‑based distro でビルド・動作可 |
| **7.6 品質**  | NF‑QUAL‑01 | OCPP 公式 Conformance Test（Core 合格）を受験し、全テスト Pass  |

---

### 8. 詳細アーキテクチャ要件

| コンポーネント               | 主な責務                                    | 実装指針                                   |
| --------------------- | --------------------------------------- | -------------------------------------- |
| **OcppClientManager** | WebSocket 管理、JSON シリアライズ／パーサ、各 EVSE 状態機 | Boost.Asio / websocketpp など非同期ライブラリを使用 |
| **MappingEngine**     | 変数・コマンド変換、状態検知、スケーリング                   | YAML/JSON ファイルを読み込みメモリ上に展開。変更監視付き      |
| **ElAdapter**         | UDP ソケット管理、EHD フレーム組立・解析、自動再送           | 同期 I/O＋タイムスタンプでレスポンス一致確認               |
| **ModbusAdapter**     | RTU/TCP 両対応、要求キュー、CRC 検証、エラー判定          | libmodbus ラッパー＋自前ステートマシン               |
| **AdminAPI**          | REST/CLI でデバイス追加・設定変更・メトリクス公開           | OpenAPI (Swagger) 定義＋認証トークン            |

---

### 9. 通信インタフェース仕様（要約）

| 種別         | 物理層/ポート               | プロトコル                     | 冗長化          | 備考                       |
| ---------- | --------------------- | ------------------------- | ------------ | ------------------------ |
| OCPP       | Ethernet/IP, 443/TCP  | WebSocket over TLS1.2/1.3 | OS 標準 TCP 再送 | 換装不要                     |
| EL         | Ethernet/IP, 3610/UDP | ECHONET Lite              | アプリ層で再送      | マルチキャスト探索使用              |
| Modbus TCP | Ethernet/IP, 502/TCP  | Modbus TCP                | TCP 再送       | 短時間 Keep‑Alive           |
| Modbus RTU | RS‑485 2‑Wire         | Modbus RTU                | なし（アプリ層）     | 9600–115200 bps, 停止ビット 1 |

---

### 10. マッピング定義方式

1. **テンプレートファイル（device\_template/\*.yaml）**

   * *例* `vendorX_modelY.yaml`
   * OCPP 変数ごとに `{type: Modbus, reg: 40010, scale: 0.1, enum: {0:Available,1:Charging}}` を定義
2. **システム設定ファイル（system\_config.yaml）**

   * 充電器 ID／IP／SlaveID、利用テンプレートを紐付け
3. **ホットリロード**

   * `SIGHUP` または REST `/reload` でマッピング再読込
4. **検証ツール**

   * CLI コマンド `mapping test <device_id>`：実機（またはシミュレータ）にアクセスし、読取値 → OCPP JSON を生成して表示

---

### 11. フェーズ別機能実装計画（抜粋）

| フェーズ        | 主要リリース内容                                                                                  |
| ----------- | ----------------------------------------------------------------------------------------- |
| **Phase 1** | Core + RemoteStart/Stop + MeterValues + 基本 SmartCharging、Modbus/EL アダプタ、MappingEngine、TLS |
| **Phase 2** | LocalAuthList、Reservation、FirmwareUpdate（ゲートウェイ自身）、Get/SetVariables、マルチ RS‑485 サポート       |
| **Phase 3** | 監視 UI（Grafana 連携）、表示メッセージ、Advanced Security（証明書管理）、Plug\&Charge 検討                        |

---

### 12. テスト・受入基準（概要）

| 種別            | 内容                        | 合格基準                       |
| ------------- | ------------------------- | -------------------------- |
| **単体試験**      | モジュールごとの関数・例外・境界値         | 100 % 必須パス                 |
| **結合試験**      | OCPP ↔ Mapping ↔ デバイス通信統合 | 正常系／異常系 95 % パス            |
| **性能試験**      | 100 台接続・取引同時 10 件         | CPU < 60 %, レイテンシ < 500 ms |
| **OCPP 適合試験** | 公式テストツール                  | Core Profile 100 % パス      |
| **フィールド試験**   | 実機環境で 72h 連続稼働            | 取引落ち・ハング 0 件               |

---

### 13. 制約条件・前提

1. 充電器が公開するレジスタ／EPC に機能が実装されていない場合、該当 OCPP 機能は **NotSupported** 応答で回避
2. RS‑485 配線長・終端抵抗等は現場設計範囲
3. サーバ証明書発行・運用は CSMS 事業者側の責任
4. 本ゲートウェイへは root 権限不要（setcap で権限実行）

---

### 14. 将来拡張の方向性

* OCPP 2.1 へのアップグレード互換
* ISO 15118 Plug\&Charge 統合（必要に応じ EVCC 連携）
* MQTT/REST ブリッジ機能追加により、EMS とのデータ連携

---

## まとめ

本要件定義書は、既存の EL／Modbus 充電器を OCPP 2.0.1 に“**もれなく**”接続するための機能・性能・保守・拡張観点を網羅した。開発フェーズに入る際は、本書をベースに詳細設計書・実装ガイドライン・試験仕様書を策定し、段階的にリリースすることで確実な導入効果を得られる。
