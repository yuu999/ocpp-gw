# OCPP 2.0.1 ゲートウェイ・ミドルウェア

既存のECHONET Lite（EL）およびModbus（RTU/TCP）対応充電器をOCPP 2.0.1ネットワークに統合するミドルウェアソリューションです。

## 機能

- OCPP 2.0.1 Core Profile完全実装
- ECHONET LiteおよびModbus（RTU/TCP）プロトコル対応
- OCPP変数とデバイス固有レジスタ/EPC間の動的マッピング
- 最大100台の充電器を同時管理可能
- TLS 1.2/1.3による安全な通信
- 包括的なログ出力と監視機能
- Webベース管理インタフェース
- 国際化対応（英語・日本語）

## 必要条件

- C++17対応コンパイラ
- CMake 3.16以上
- Boostライブラリ（system, filesystem, thread, program_options）
- OpenSSL
- yaml-cpp
- jsoncpp
- libmodbus
- spdlog

## ソースからのビルド

### 依存関係のインストール

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libboost-all-dev libssl-dev libyaml-cpp-dev libjsoncpp-dev libmodbus-dev libspdlog-dev
```

### ビルド

```bash
mkdir build
cd build
cmake ..
make
```

#### Windows (MSVC)

```powershell
# 依存ライブラリのインストール (vcpkgを使用)
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
vcpkg.exe install boost-asio:x64-windows boost-beast:x64-windows yaml-cpp:x64-windows jsoncpp:x64-windows spdlog:x64-windows openssl:x64-windows libmodbus:x64-windows
cd ..

# ビルド
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="../vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build . --config Release
ctest -C Release
```

### テスト実行

```bash
cd build
ctest
```

### インストール

```bash
cd build
sudo make install
```

## 設定

インストール後、設定ファイルは `/etc/ocpp-gw/` に保存されます：

- `system.yaml`: メインシステム設定
- `templates/`: デバイステンプレート設定
- `devices/`: 個別デバイス設定

## 使用方法

```bash
ocpp-gateway --config /path/to/system.yaml
```

## 開発

### プロジェクト構造

```text
プロジェクトトップ
├── include/                # Public C++ headers
├── src/                    # Implementation sources
├── config/                 # YAML configuration & templates
├── docs/                   # Guides & reference docs
├── tests/                  # Unit & integration tests
├── examples/               # Sample programs
├── scripts/                # Helper scripts & service files
├── work_log/               # Development history logs
├── simulators/             # Test simulators for development
│   ├── elemu/             # Reference ECHONET Lite emulator
│   └── echonet-simulator/ # New ECHONET Lite simulator
├── third_party/            # External dependencies (vendored or submodules)
└── CMakeLists.txt          # Top-level build script
```

### 新しいデバイステンプレートの追加

1. `config/templates/` に新しいテンプレートファイルを作成
2. OCPP変数とデバイスレジスタ/EPC間のマッピングを定義
3. デバイス設定ファイルでテンプレートを参照

## ライセンス

[LGPL-3.0 License](LICENSE)