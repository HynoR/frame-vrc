# インストールとコマンド一覧

[English](install.md) · [中文](install.zh-CN.md) · **日本語**

画像付きの手順は [インストール・利用チュートリアル](tutorial.ja.md) を参照してください。すべてのコマンドは Steam Frame 上（Konsole または SSH）で通常ユーザー `steamos` として実行します（**`sudo` は使用不可**）。

## frame-vrc のインストール

### オンラインインストール

```sh
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash
```

`get.sh` は最新リリースと `SHA256SUMS` をダウンロードし、チェックサム検証後に `install.sh` を実行します。

### リリースアーカイブからのインストール

ダウンロード済みの tarball と `SHA256SUMS` があるディレクトリで以下を実行します：

```sh
sha256sum -c --ignore-missing SHA256SUMS
tar xzf frame-vrc-<version>.tar.gz && frame-vrc-<version>/install.sh
```

本体は `~/.local/lib/frame-vrc` に配置され、`~/.local/bin/frame-vrc` にシンボリックリンクが作成されます（コマンドが見つからない場合は新しいシェルを開いてください）。あわせて VR パネルのビルドが実行されます（約 1 分）。

## `setup` の処理内容

`frame-vrc setup`（またはパネルの **Setup to PCVR Version**）は以下の処理を順に実行します：

1. **Steam 依存コンポーネントの確認**：FEX-Emu、Proton Experimental、Proton EasyAntiCheat Runtime の有無を確認し、不足があれば Steam のインストール画面を開きます。
2. **パッケージの取得と検証**：GitHub Releases からパッチ版 FEX、DFR DXVK、H.264 動画プラグイン、Wine 音声修正を取得し、SHA-256 チェックサムを検証します。
3. **専用 FEX 環境の構築**：VRChat 専用のプライベート FEX ディレクトリ（`~/.local/share/frame-vrc/fex`）に ji10me 氏のランタイムを構成し、セッション管理と視線追跡ブリッジをビルドします。
4. **専用 Proton ツールの作成**：DFR DXVK を組み込んだ Proton Experimental のコピー `Proton Experimental (Ji10me PCVR Edit)` を `~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr`（約 1.5 GB）に作成します。初回は互換ツール一覧へ反映するため Steam を 1 回再起動します。
5. **VRChat の起動設定**：VRChat の互換ツールに上記 Proton コピーを選択し、描画解像度（`1920`）と起動オプション（`frame-vrc-run %command%`）を設定します。

## コマンド一覧

```text
frame-vrc setup                  PCVR 版のセットアップと切り替え
frame-vrc official               公式 Android 版へ戻す
frame-vrc status                 現在のバージョンと各コンポーネントの状態を表示
frame-vrc doctor                 動作環境を診断し修復方法を表示（正常時は all set）
frame-vrc resolution 1056|1440|1920  片目あたりの描画解像度を設定
frame-vrc launch                 VRChat を起動
frame-vrc update                 frame-vrc 本体と各パッケージを更新
frame-vrc uninstall [--purge]    公式版に戻し frame-vrc をアンインストール
frame-vrc dfrtest [on|off]       DFR デバッグ表示の切り替え（視線インジケーター表示・DFR ログ記録）
frame-vrc video check            動画再生中に H.264 ハードウェアデコードが有効か確認
```

## メモリ節約のための VRChat 推奨設定

16 GB メモリの枯渇を防ぐため、VRChat 内で以下の設定を推奨します：
- **アバター制限**：Poor / Very Poor アバターを非表示にし、最大表示アバター数、ダウンロードサイズ上限、展開後サイズ上限を低く設定する。
- **グラフィック設定**：パーティクル制限（Particle Limiter）を有効にし、ミラーは低画質またはオフ、アンチエイリアスはオフ、その他の画質項目は Low に設定する。

## トラブルシューティング

ゲームが起動しない場合：
1. `frame-vrc doctor` を実行し、表示される指示に従って修復してください。
2. 詳細ログを取得する場合は、VRChat の起動オプションの `%command%` の前に一時的に `PROTON_LOG=1` を追加（`PROTON_LOG=1 frame-vrc-run %command%`）して起動し、`~/steam-438100.log` を確認してください（確認後は `PROTON_LOG=1` を削除してください）。
3. 問題を報告する際は、`frame-vrc status` と `frame-vrc doctor` の出力を添付してください。

変更されるファイルの一覧やアンインストール手順については [変更内容とアンインストール](changes.ja.md) を参照してください。
