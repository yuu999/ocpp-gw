# 貢献ガイドライン

OCPP Gateway Middlewareプロジェクトへの貢献をお考えいただき、ありがとうございます！このガイドでは、プロジェクトへの貢献方法について説明します。

## 行動規範

このプロジェクトに参加する全ての人は、[行動規範](CODE_OF_CONDUCT.md)に従うことが求められます。

## 貢献の方法

### バグレポート

バグを発見した場合は、以下の情報を含むIssueを作成してください：

- バグの詳細な説明
- 再現手順
- 期待される動作
- 実際の動作
- 環境情報（OS、コンパイラ、バージョンなど）
- 関連するログファイル

### 機能リクエスト

新機能のリクエストがある場合は、以下を含むIssueを作成してください：

- 機能の説明
- 使用ケース
- 実装のアイデア（あれば）

### プルリクエスト

#### 開発環境の設定

1. リポジトリをフォーク
2. ローカルにクローン
```bash
git clone https://github.com/yourusername/ocpp-gw.git
cd ocpp-gw
```

3. 依存関係のインストール（詳細はREADME.mdを参照）

4. ビルドとテストの実行
```bash
mkdir build
cd build
cmake ..
make
ctest
```

#### ブランチ戦略

- メインブランチ: `main`
- 機能開発: `feature/feature-name`
- バグ修正: `bugfix/bug-description`
- ホットフィックス: `hotfix/issue-description`

#### プルリクエストの手順

1. 新しいブランチを作成
```bash
git checkout -b feature/your-feature-name
```

2. 変更を実装
3. テストを追加/更新
4. ローカルでテストを実行
```bash
cd build
make
ctest
```

5. コミット
```bash
git add .
git commit -m "feat: add new feature description"
```

6. プッシュ
```bash
git push origin feature/your-feature-name
```

7. GitHubでプルリクエストを作成

#### コミットメッセージ規約

[Conventional Commits](https://www.conventionalcommits.org/)形式を使用してください：

```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

Types:
- `feat`: 新機能
- `fix`: バグ修正
- `docs`: ドキュメントのみの変更
- `style`: コード動作に影響しない変更（スペース、フォーマットなど）
- `refactor`: バグ修正や機能追加を伴わないコードの変更
- `test`: テストの追加や修正
- `chore`: ビルドプロセスやツールの変更

例:
```
feat(ocpp): add transaction event handling
fix(device): resolve modbus connection timeout issue
docs: update installation instructions
```

## コーディング規約

### C++

- C++17標準に準拠
- インデントは4スペース
- 行の長さは100文字以内
- クラス名は PascalCase
- 関数名・変数名は snake_case
- 定数は UPPER_CASE
- ヘッダーファイルは `.h` 拡張子
- ソースファイルは `.cpp` 拡張子

### ドキュメント

- すべてのpublic APIにはDoxygenコメントを追加
- READMEは常に最新に保つ
- 設計変更時は design.md を更新

### テスト

- 新機能には対応するテストを追加
- テストカバレッジは80%以上を維持
- 単体テストと統合テストの両方を考慮

## レビュープロセス

1. すべてのプルリクエストは、最低1人のメンテナーによるレビューが必要
2. CI/CDパイプラインがすべて通ること
3. コードカバレッジが維持されること
4. ドキュメントが適切に更新されること

## 質問・サポート

- 一般的な質問: GitHub Discussions
- バグレポート: GitHub Issues
- セキュリティの問題: SECURITY.md を参照

## ライセンス

このプロジェクトに貢献することで、あなたの貢献がLGPL-3.0 Licenseの下で公開されることに同意したものとみなします。 