# C++17互換性修正作業ログ

## 日時
2024年12月21日 14:00

## 問題
CI/CDビルドで以下のエラーが発生：
```
error: 'std::string' {aka 'class std::__cxx11::basic_string<char>'} has no member named 'starts_with'
```

## 原因
`std::string::starts_with`メソッドはC++20で導入された機能のため、C++17環境では使用できない。

## 修正内容
`src/common/metrics_collector.cpp`の437行目と444行目で使用されていた`starts_with`を`find`メソッドを使用したC++17互換の実装に変更。

### 修正前
```cpp
if (line.starts_with("MemTotal:")) {
    // ...
} else if (line.starts_with("MemAvailable:")) {
    // ...
}
```

### 修正後
```cpp
if (line.find("MemTotal:") == 0) {
    // ...
} else if (line.find("MemAvailable:") == 0) {
    // ...
}
```

## 影響
- C++17環境でのビルドが正常に動作するようになる
- 機能的な変更はない（同じ動作を保証）

## 確認事項
- 他のファイルで`starts_with`の使用箇所がないことを確認済み
- 修正によりC++17互換性が確保された 