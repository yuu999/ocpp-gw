# エコネットマッピング修正作業ログ

## 日時
2024年12月21日 13:00

## 作業内容
エコネットのマッピング機能を修正し、OCPPの目的に合わせて充電器関連のクラス（EV充電器、EV充放電器）に特化した機能開発を実施

## 問題点
1. **クラス定義の間違い**: 現在 `0x7D` を充電器クラスとして使用していたが、正しいクラス定義が不明確
2. **テンプレート設定の不整合**: テンプレートでは `0x87` (蓄電池クラス) を使用していたが、アダプターでは `0x7D` を検索
3. **OCPPの目的から外れたクラス**: 蓄電池や太陽光発電はOCPPの本来の目的（充電器管理）から外れている
4. **充電器と充放電器の区別がない**: 両方のクラスに対応する機能が不足

## 修正内容

### 1. エコネットクラス定義の修正（正しい仕様に基づく）
- `src/device/echonet_lite_adapter.cpp` に正しいクラス定義を追加
- `include/ocpp_gateway/device/echonet_lite_adapter.h` にも定義を追加

```cpp
// ECHONET Lite EV charger classes (corrected based on ECHONET specification)
constexpr uint8_t EOJ_EV_CHARGER_CLASS_GROUP = 0x02;  // 住宅・設備関連機器クラスグループ
constexpr uint8_t EOJ_EV_CHARGER_CLASS = 0xA1;        // 電気自動車充電器クラス
constexpr uint8_t EOJ_EV_DISCHARGER_CLASS = 0xA1;     // 電気自動車充電器クラス（充放電対応）

// ECHONET Lite EV charger specific properties
constexpr uint8_t EPC_OPERATION_MODE_SETTING = 0xDA;  // 運転モード設定
```

### 2. デバイス発見処理の修正
- 複数のデバイスクラスに対応するよう修正
- 各クラスに応じたテンプレートIDを設定

### 3. ステータス監視処理の修正
- 複数のデバイスクラスに対応するよう修正
- 各クラスを順次試行してオンライン状態を確認

### 4. テンプレートファイルの作成（OCPP特化）
以下の2つのテンプレートファイルを新規作成：

#### a. EV充電器専用テンプレート (`config/templates/echonet_lite_charger.yaml`)
- クラス: `0xA1` (電気自動車充電器クラス)
- 充電専用の機能に特化
- 運転モード設定（EPC: 0xDA）に対応

#### b. EV充放電器専用テンプレート (`config/templates/echonet_lite_charger_discharger.yaml`)
- クラス: `0xA1` (電気自動車充電器クラス)
- 充電・放電・充放電・自動・停止・待機の全モードに対応
- 運転モード設定（EPC: 0xDA）に対応

### 5. テストファイルの更新
- `tests/unit/device/echonet_lite_adapter_test.cpp` を更新
- 新しいクラス定義に対応

## 技術的詳細

### エコネットクラス定義
- **0x7D**: EV充電器クラス - 充電専用機能
- **0x7E**: EV充放電器クラス - 充電・放電両方の機能
- **0x87**: 蓄電池クラス - 蓄電・放電機能
- **0x88**: 太陽光発電クラス - 発電機能

### エコネットクラス定義（OCPP特化）
- **0x7D**: EV充電器クラス - 充電専用機能
- **0x7E**: EV充放電器クラス - 充電・V2G放電両方の機能

### エコネットクラス定義（正しい仕様に基づく）
- **0xA1**: 電気自動車充電器クラス - 充電・放電・充放電・自動・停止・待機の全モード対応
- **運転モード設定**: EPC 0xDA - 充電(0x42)、放電(0x43)、充放電(0x44)、自動(0x45)、停止(0x47)、待機(0x48)

### マッピング機能
各テンプレートでは以下のOCPP変数とのマッピングを定義：

1. **基本状態監視**
   - `AvailabilityState`: デバイスの利用可能状態
   - `Connector.Status`: コネクタの状態

2. **電力量計測**
   - `MeterValue.Energy.Active.Import.Register`: 充電/蓄電電力量
   - `MeterValue.Energy.Active.Export.Register`: 放電/発電電力量

3. **電力計測**
   - `MeterValue.Power.Active.Import`: 充電/蓄電電力
   - `MeterValue.Power.Active.Export`: 放電/発電電力

4. **電圧・電流計測**
   - `MeterValue.Voltage`: 電圧
   - `MeterValue.Current.Import`: 電流

5. **デバイス固有機能**
   - `OperationMode`: 運転モード設定（充電/放電/充放電/自動/停止/待機）
   - `StartTransaction`: 充電開始コマンド
   - `StopTransaction`: 充電停止コマンド
   - `StartDischarging`: 放電開始コマンド
   - `StopDischarging`: 放電停止コマンド

## 今後の課題
1. **実機テスト**: 実際のエコネットデバイスでの動作確認
2. **EPC定義の精査**: 各デバイスで使用されるEPCの正確な定義確認
3. **エラーハンドリング**: デバイスクラス判定失敗時の適切な処理
4. **パフォーマンス最適化**: 複数クラス対応による処理負荷の最適化

## 関連ファイル
- `src/device/echonet_lite_adapter.cpp`
- `include/ocpp_gateway/device/echonet_lite_adapter.h`
- `config/templates/echonet_lite_charger.yaml`
- `config/templates/echonet_lite_charger_discharger.yaml`
- `tests/unit/device/echonet_lite_adapter_test.cpp` 