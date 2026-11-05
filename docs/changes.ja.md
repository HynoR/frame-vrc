# 変更内容と元に戻す方法

[English](changes.md) · [中文](changes.zh-CN.md) · **日本語**

frame-vrc が書き込むのはホームフォルダの中だけです。触るものは以下ですべてです。

## Steam のファイルで変わるもの

| 内容 | 場所 | 変更するコマンド | 元に戻す |
|---|---|---|---|
| FEX-Emu：7 つのプログラム（`FEX`、`FEXBash`、`FEXGetConfig`、`FEXOfflineCompiler`、`FEXServer`、`FEXServerManager`、`FEXpidof`）と `FEXCompatTool`、`VERSIONS.txt`、`ConfigTemplate.json` | `~/.local/share/Steam/steamapps/common/FEX-Emu/` | `switch eac` / `switch thunk` | `frame-vrc switch valve`（バックアップからコピーし、全ファイルを比較） |
| FEX-Emu の Vulkan thunk（`thunk` ビルドのみ）：`usr/lib/aarch64-linux-gnu/fex-emu/HostThunks/libvulkan-host.so`、`usr/share/fex-emu/GuestThunks/libvulkan-guest.so` | 同上 | `switch thunk` | `switch eac` または `switch valve`（Valve の元ファイルはバックアップにある） |

バックアップを失った場合：Steam > ライブラリ > ツール > FEX-Emu > アンインストールして再インストールすれば、Valve のファイルが戻ります。
Steam が FEX-Emu を更新するとパッチ版は上書きされます。`frame-vrc switch eac` をもう一度実行してください（新しい Valve 版がバックアップされ、
古いバックアップは `valve-backup.old-<日付>` として残ります）。

## Steam / SteamVR の設定で変わるもの

| 内容 | 場所 | 変更するコマンド | 元に戻す |
|---|---|---|---|
| VRChat の起動オプション | Steam（`~/.local/share/Steam/userdata/*/config/localconfig.vdf` に保存） | `launch-options set`、`switch`（frame-vrc が設定したものの場合のみ） | `frame-vrc launch-options clear`（元の値に戻す。元の値は `~/.local/share/frame-vrc/launch-options.438100.prev`）、または VRChat > プロパティで削除 |
| VRChat の描画解像度 | SteamVR（`~/.config/openvr/config/steamvr.vrsettings`、`steam.app.438100` > `resolutionOverride`） | `resolution N`（SteamVR の `vrcmd` 経由） | `frame-vrc resolution reset`（元の値は `~/.local/share/frame-vrc/resolution.438100.prev`）、または SteamVR > 設定 > ビデオ > アプリごとの設定 |

## frame-vrc が作るもの

| 内容 | 場所 | 削除方法 |
|---|---|---|
| ツール本体 | `~/.local/lib/frame-vrc`、`~/.local/bin/frame-vrc` | `frame-vrc uninstall` |
| **Valve 版 FEX のバックアップ**（残しておく） | `~/fex-eac-work/valve-backup/FEX-<バージョン>/`（+ `thunks/`） | `uninstall --purge`。Valve 版に戻した後でのみ削除 |
| ビルドとソース | `~/fex-eac-work/`（`stage`、`stage-mod`、`*.prev`、`FEX`、`build-*`、`build.log`） | `uninstall --purge` |
| thunk 用のドライバ一覧 | `~/.local/share/frame-vrc/vk/freedreno_host.aarch64.json` | `uninstall --purge` |
| ベンチマークのレポート、パッケージ | `~/.local/share/frame-vrc/bench/`、`~/.local/share/frame-vrc/dist/` | `uninstall --purge`、または削除 |

## プレイ中にできるもの（frame-vrc ではなく Steam、FEX、Proton が作る）

| 内容 | 場所 | 削除 |
|---|---|---|
| FEX のコードキャッシュ（数回で約 0.8 GB。2 回目以降の起動を速くする） | `~/.local/share/Steam/steamapps/shadercache/438100/fex-emu/` | フォルダを削除すれば再作成される |
| VRChat の Wine プレフィックス（EAC のファイル、ゲーム設定、キャッシュ） | `~/.local/share/Steam/steamapps/compatdata/438100/` | Steam で VRChat をアンインストール |

## 触らないもの

システム（`/usr`、`/etc`、`/opt`）、ファームウェア、`/persist`、ほかのパーティション、Proton、EAC Runtime、VRChat のゲームファイル、ほかのゲーム。
ネイティブや Proton ARM64 のゲームは FEX を使いません。

## 完全アンインストール

先にゲームを終了し（FEX が動いていないこと）、SteamVR は起動したまま（解像度を戻すため）：

```sh
frame-vrc uninstall           # Valve の FEX に戻し、起動オプションを消し、解像度を戻し、ツールを削除。ビルド、バックアップ、レポートは残す
frame-vrc uninstall --purge   # ~/fex-eac-work と ~/.local/share/frame-vrc も削除
```

Proton Experimental、EAC Runtime、VRChat は Steam に残ります。VRChat の互換性をデフォルトに戻せば Android 版に戻ります。

### 手動（方法 C でインストールした場合）

```sh
sh install-fex-tool.sh restore                       # ~/fex-eac-work/valve-backup から Valve の FEX に戻す
/opt/steamvr/bin/linuxarm64/vrcmd --set-settings-int steam.app.438100.resolutionOverride <元の値>
rm -rf ~/fex-eac-work                                # 先に `sh install-fex-tool.sh status` で確認
```

最後に VRChat > プロパティで起動オプションを削除してください。
