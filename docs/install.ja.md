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

`frame-vrc setup`（またはパネルの **Setup to PCVR Version**）は 8 ステップのウィザードを順に実行します。途中で中断・失敗した場合は、再度 `frame-vrc setup`（またはパネルの **Retry This Step**）を実行すると未完了のステップから再開します：

1. **Steam と VRChat の確認**：Steam の応答と VRChat がライブラリにあるかを確認します（未追加時はストア案内、起動中は終了案内を表示）。
2. **FEX-Emu のインストール**：FEX-Emu（`3127680`）を確認し、未導入なら Steam のインストール画面を開いて確認・完了を待ちます。
3. **Proton Experimental のインストール**：前の項目が準備できてから Proton Experimental（`1493710`）のインストール画面を順に開き、完了を待ちます。
4. **Proton EasyAntiCheat Runtime のインストール**：同様に 1 つずつ確認しながら Proton EasyAntiCheat Runtime（`1826330`）を導入します。
5. **パッケージの取得と検証**：パッチ版 FEX、DFR DXVK、H.264 動画プラグイン、Wine 音声修正を取得（またはローカル指定ファイルを使用）し、SHA-256 チェックサムを検証します。
6. **PCVR ランタイムの配置**：セッション管理と視線追跡ブリッジをビルドし、VRChat 専用のプライベート FEX（`~/.local/share/frame-vrc/fex`）と `Proton Experimental (Ji10me PCVR Edit)`（`~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr`、約 1.5 GB）を配置します。
7. **Steam に Proton を認識させる**：新しい Proton コピーが Steam に登録されているか確認し、未登録の場合は Steam を 1 回再起動（画面が一時的に消えます）して再確認します。
8. **VRChat 設定の適用と確認**：互換ツール、片目あたりの描画解像度（`1920`）、起動オプション（`frame-vrc-run %command%`）を設定し、VRChat 本体のインストール／更新が Steam 側で必要な場合はその状態を表示します。

## コマンド一覧

```text
frame-vrc setup                  PCVR 版のセットアップと切り替え（未完了時は続きから再開）
frame-vrc install-deps           Steam 依存ツールのみを導入（ステップ 1–4）
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
