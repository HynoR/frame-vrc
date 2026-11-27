# インストール・利用チュートリアル

[English](tutorial.md) · [中文](tutorial.zh-CN.md) · **日本語**

Steam Frame に `frame-vrc` を導入し、PC 版 VRChat（EasyAntiCheat 対応）を起動するまでの手順を説明します。所要時間は約 10 分です（別途約 1.1 GB のゲーム更新が必要です）。事前に [README](../README.ja.md) の免責事項をご確認ください。

## バージョン切り替えの仕組み

`frame-vrc` は Steam の互換ツール設定を変更することで VRChat のバージョンを切り替えます：

| バージョン | Steam 互換ツール |
|---|---|
| **公式版**（Android ビルド） | デフォルト（Lepton） |
| **PCVR 版**（Windows ビルド + DFR + H.264 ハードウェア動画） | Proton Experimental (Ji10me PCVR Edit) |

VRChat の起動時に、選択されている互換ツールに対応するビルドへ Steam が自動更新します。パッチ版 FEX は VRChat 専用環境として分離されており、Steam 標準の FEX-Emu や他のゲームには影響しません。

## 1. frame-vrc のインストール

ヘッドセットの Konsole（または SSH）で、通常ユーザー `steamos` として以下のコマンドを実行します（**`sudo` は使用しないでください**）：

```sh
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash
```

約 1 分で完了します。完了後、SteamVR ダッシュボードの **+**（プログラムを起動）> **frame-vrc** から VR パネルを開きます。

![SteamVR ダッシュボード：+（プログラムを起動）> frame-vrc](images/steamvr-launch-program.jpg)

## 2. PCVR 版のセットアップ

パネルの **Install** ページを開きます：

![Install ページ：Setup to PCVR Version](images/panel-install-setup.jpg)

1. **Setup to PCVR Version**（緑）をタップし、もう一度タップして確定します。**Install** ページが 8 ステップのセットアップウィザードに切り替わります（**View log** で生ログも確認できます）。
2. 不足している Steam ツール（FEX-Emu、Proton Experimental、Proton EasyAntiCheat Runtime）は **1 つずつ**順にインストール確認が表示されます。画面を閉じてしまった場合は **Reopen Install Prompt** で再度開けます。Steam のダウンロード中は自動で待機します。
3. ステップ 7 では、新しい互換ツールを認識させるため必要に応じて **Steam が 1 回再起動**します（画面が一時的に消え、パネルを開き直すと自動で再開します）。
4. 途中で失敗・停止した場合は、**Retry This Step**（または `frame-vrc setup` の再実行）でそのステップから再開できます。

![セットアップ中の Log ページ](images/panel-log.jpg)

完了後、VRChat 本体のダウンロードや PC ビルドへの更新がまだ必要な場合は、Steam でのインストール／更新が必要である旨（または **patched: update and launch VRChat**）が表示されます。

## 3. VRChat の起動

パネルの **Home** ページまたは Steam から VRChat を起動します。初回は PC ビルド（約 1.1 GB）のダウンロードが行われた後に起動します。最初のワールド入室時はシェーダー処理のため数分かかりますが、2 回目以降は高速になります。

![Steam が VRChat を PC ビルドに更新中](images/steam-vrchat-updating.jpg)

**VRChat > プロパティ > 互換性** で設定内容を確認できます（起動オプションは自動で `frame-vrc-run %command%` に設定されます。手動で変更しないでください）：

![互換ツール：Proton Experimental (Ji10me PCVR Edit)](images/compat-pcvr-version.jpg)

## 4. パネルの各ページ

左下の言語ボタンで English / 日本語 を切り替えられます。VRChat が未インストールまたは公式版の状態では **Home** と **Install** のみ表示され、PCVR 版に切り替えると全ページが表示されます：

- **Home**：メモリ・CPU 使用率、描画解像度（`1056` / `1440` / `1920`、既定値 `1920`）、**Launch VRChat** ボタン。中心窩レンダリング（DFR）と H.264 ハードウェアデコードは常時有効です。

  ![Home ページ](images/panel-home.jpg)

- **Environment**：動作環境のチェック項目一覧。問題がある項目には修復ボタンが表示されます。

  ![Environment ページ](images/panel-environment.jpg)

- **Advanced**：導入済みコンポーネントのバージョン、追加の解像度設定、スワップ状態、アップデート・アンインストール操作。

  ![Advanced ページ](images/panel-advanced.jpg)

## 5. 公式 Android 版に戻す

**Install** ページで **Recover to Official Android Version**（赤）をタップし、もう一度タップして確定します。互換ツールがデフォルト（Lepton）に戻り、元の起動オプションと解像度設定が復元されます。次回起動時に Steam が Android ビルドを再ダウンロードします。

![Install ページ：Recover to Official Android Version](images/panel-install-recover.jpg)

![互換ツールがデフォルト（Lepton）に戻った状態](images/compat-official-lepton.jpg)

## コマンドライン操作

パネルの全機能はコマンドラインからも実行できます：

```text
frame-vrc setup | official | status | doctor
frame-vrc resolution 1056|1440|1920
frame-vrc launch | update | uninstall [--purge]
```

詳細は [インストールとコマンド一覧](install.ja.md) および [変更内容とアンインストール](changes.ja.md) を参照してください。
