# 変更内容と元に戻す方法

[English](changes.md) · [中文](changes.zh-CN.md) · **日本語**

frame-vrc はホームフォルダ以下にだけ書き込みます。触るものはこれですべてです。

## Steam のファイルで変わるもの

| 内容 | 場所 | 変更するコマンド | 戻し方 |
|---|---|---|---|
| FEX-Emu：7 つのプログラム（`FEX`、`FEXBash`、`FEXGetConfig`、`FEXOfflineCompiler`、`FEXServer`、`FEXServerManager`、`FEXpidof`）と `FEXCompatTool`、`VERSIONS.txt`、`ConfigTemplate.json` | `~/.local/share/Steam/steamapps/common/FEX-Emu/` | `fex eac` | `frame-vrc fex valve`（バックアップからコピーし、全ファイルを比較） |

バックアップをなくした場合：Steam > ライブラリ > ツール > FEX-Emu > アンインストールして再インストールすると、Valve のファイルに戻ります。
Steam が FEX-Emu を更新するとパッチ版は上書きされます。`frame-vrc fex eac` をもう一度実行してください（新しい Valve 版がバックアップされ、
古いバックアップは `valve-backup.old-<日付>` として残ります）。

Proton Experimental 本体は一切変更しません。DFR は複製を使います（下記）。

## Steam / SteamVR の設定で変わるもの

| 内容 | 場所 | 変更するコマンド | 戻し方 |
|---|---|---|---|
| VRChat の起動オプション | Steam（`~/.local/share/Steam/userdata/*/config/localconfig.vdf` に保存） | `launch-options set` と各スイッチ（frame-vrc が設定したものだけ） | `frame-vrc launch-options clear`（元の設定に戻す。元の値は `~/.local/share/frame-vrc/launch-options.438100.prev`）、または VRChat > プロパティで削除 |
| VRChat の互換性ツール | Steam（`~/.local/share/Steam/config/config.vdf`、`CompatToolMapping`） | `proton dfr` / `proton stock` | `frame-vrc proton stock`、または VRChat > プロパティ > 互換性 > Proton Experimental |
| VRChat の描画解像度 | SteamVR（`~/.config/openvr/config/steamvr.vrsettings`、`steam.app.438100` > `resolutionOverride`） | `resolution N`（SteamVR の `vrcmd` 経由） | `frame-vrc resolution reset`（元の値は `~/.local/share/frame-vrc/resolution.438100.prev`）、または SteamVR > 設定 > ビデオ > アプリケーションごと |

## frame-vrc が作るもの

| 内容 | 場所 | 削除方法 |
|---|---|---|
| ツール本体 | `~/.local/lib/frame-vrc`、`~/.local/bin/frame-vrc` | `frame-vrc uninstall` |
| VR パネルのアプリ一覧の項目 | `~/.local/share/applications/frame-vrc.desktop`（パネル本体は `~/.local/lib/frame-vrc`） | `frame-vrc uninstall` |
| **Valve 版 FEX のバックアップ**（残しておく） | `~/fex-eac-work/valve-backup/FEX-<バージョン>/` | `uninstall --purge`（Valve 版に戻した後だけ） |
| パッチ版 FEX（ヘッドセットでビルドした場合はソースも） | `~/fex-eac-work/`（`stage`、`stage.prev`、`FEX`、`build-*`、`build.log`） | `uninstall --purge` |
| DFR 版 Proton（Proton Experimental の複製に DFR 版 DXVK を入れたもの。元のファイルは `dxvk-orig/`） | `~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr/`（約 1.5 GB） | `frame-vrc uninstall`、または `proton stock` の後にフォルダを削除 |
| 設定、DFR パッケージ、ブリッジのログ、ベンチマークレポート | `~/.local/share/frame-vrc/`（`config`、`dfr/`、`bridge.log`、`bench/`、`dist/`、`panel/`：パネルの最後の操作とその出力） | `uninstall --purge`、または削除 |
| ブリッジの視線ファイル | `~/.local/share/frame-pcvr/gaze/frame-gaze-v1.bin`（64 バイト、ブリッジ停止時に空になる） | フォルダを削除 |

## プレイ中に作られるもの（Steam、FEX、Proton が作る。frame-vrc ではない）

| 内容 | 場所 | 削除 |
|---|---|---|
| FEX のコードキャッシュ（数回で約 1 GB。次回以降の起動を速くする） | `~/.local/share/Steam/steamapps/shadercache/438100/fex-emu/` | フォルダを削除すれば作り直される |
| VRChat の Wine プレフィックス（EAC のファイル、ゲーム設定、キャッシュ） | `~/.local/share/Steam/steamapps/compatdata/438100/` | Steam で VRChat をアンインストール |

## 触らないもの

システム（`/usr`、`/etc`、`/opt`）、ファームウェア、`/persist`、ほかのパーティション、Proton Experimental 本体、EAC Runtime、VRChat のゲームファイル、
ほかのゲーム。ネイティブのゲームと Proton ARM64 のゲームは FEX を使いません。

## 完全アンインストール

先にゲームを終了し（FEX が動いていないこと）、SteamVR は起動したまま（解像度を戻すため）：

```sh
frame-vrc uninstall           # Valve の FEX に戻し、VRChat を Proton Experimental に、DFR の複製を削除、起動オプションを消去、
                              # 解像度を戻し、ツールを削除。FEX のビルド、バックアップ、設定、レポートは残す
frame-vrc uninstall --purge   # ~/fex-eac-work と ~/.local/share/frame-vrc も削除
```

Proton Experimental、EAC Runtime、VRChat は Steam に残ります。VRChat の互換性を既定に戻せば Android 版に戻ります。

### 手動で

```sh
sh install-fex-tool.sh restore                       # ~/fex-eac-work/valve-backup から Valve の FEX に戻す
/opt/steamvr/bin/linuxarm64/vrcmd --set-settings-int steam.app.438100.resolutionOverride <元の値>
rm -rf ~/fex-eac-work ~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr   # `install-fex-tool.sh status` で確認してから
```

その後、VRChat > プロパティ > 互換性を Proton Experimental に戻し、起動オプションを削除してください。
