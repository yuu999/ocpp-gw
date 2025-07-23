cmake_minimum_required(VERSION 3.16)
project(ocpp-gateway VERSION 1.0.0 LANGUAGES CXX)

# CMakeポリシーの設定
# cmake_policy(SET CMP0167 NEW)  # CMake 3.27以降で利用可能

# C++17を使用
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# コンパイラオプション
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# ビルドタイプのデフォルト設定
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 依存ライブラリの検出
# find_package(PkgConfig REQUIRED)  # Windows環境では無効化
find_package(Boost REQUIRED COMPONENTS system filesystem thread program_options)
find_package(Threads REQUIRED)

# OpenSSL
find_package(OpenSSL REQUIRED)

# yaml-cpp
find_package(yaml-cpp REQUIRED)

# jsoncpp
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# libmodbus
# pkg_check_modules(LIBMODBUS REQUIRED libmodbus)  # Windows環境では無効化

# spdlog
find_package(spdlog REQUIRED)

# プロジェクトのインクルードディレクトリ
include_directories(${CMAKE_SOURCE_DIR}/include)
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party")
    include_directories(${CMAKE_SOURCE_DIR}/third_party)
endif()

# サブディレクトリ
add_subdirectory(src)

# third_partyディレクトリが存在する場合のみ追加
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/CMakeLists.txt")
    add_subdirectory(third_party)
endif()

# テストの有効化（オプション）
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# インストール設定
install(TARGETS ocpp-gateway
    DESTINATION bin
)

# 設定ファイルのインストール
install(DIRECTORY config/
    DESTINATION etc/ocpp-gw
    FILES_MATCHING PATTERN "*.yaml"
)

# systemdサービスファイルのインストール
install(FILES scripts/ocpp-gateway.service
    DESTINATION lib/systemd/system
)

# パッケージ情報
set(CPACK_PACKAGE_NAME "ocpp-gateway")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OCPP 2.0.1 Gateway for EL/Modbus chargers")
set(CPACK_PACKAGE_VENDOR "OCPP Gateway Project")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "OCPP Gateway Team")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-all-dev, libssl-dev, libyaml-cpp-dev, libjsoncpp-dev, libmodbus-dev, libspdlog-dev")

include(CPack) 