# 変更内容とアンインストール

[English](changes.md) · [中文](changes.zh-CN.md) · **日本語**

`frame-vrc` はユーザーのホームディレクトリ（`~`）内にのみ書き込みを行い、`sudo` は一切使用しません。変更される設定と作成されるファイルは以下のとおりです。

## Steam・SteamVR の設定変更（VRChat のみ）

| 設定項目 | 変更コマンド | 復元方法 |
|---|---|---|
| VRChat の互換ツール：`Proton Experimental (Ji10me PCVR Edit)` | `setup` | `official`（デフォルト / `Lepton` に戻す） |
| VRChat の起動オプション：`frame-vrc-run %command%` | `setup` | `official`（`~/.local/share/frame-vrc/launch-options.438100.prev` のバックアップから復元） |
| SteamVR の VRChat 描画解像度 | `setup`、`resolution N` | `official`（`~/.local/share/frame-vrc/resolution.438100.prev` のバックアップから復元） |

## 作成されるファイルとディレクトリ

| 内容 | パス |
|---|---|
| CLI ツールおよび VR パネル | `~/.local/lib/frame-vrc`、`~/.local/bin/frame-vrc`、`~/.local/share/applications/frame-vrc.desktop` |
| Proton コピー（約 1.5 GB） | `~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr/` |
| プライベート FEX 環境、パッケージ、設定データ | `~/.local/share/frame-vrc/` |
| FEX パッケージのキャッシュ | `~/fex-eac-work/` |
| 視線追跡ブリッジの視線データ（64 バイト） | `~/.local/share/frame-pcvr/gaze/` |
| VRChat 用 FEX Thunk 設定 | `~/.local/share/Steam/steamapps/compatdata/438100/fex-emu/ThunksDB.json` |

## 変更されない領域

以下の要素には一切変更を加えません：
- システムディレクトリ（`/usr`、`/etc`、`/opt`）、ファームウェア、他パーティション
- Steam 標準の FEX-Emu、Proton Experimental 本体、Proton EasyAntiCheat Runtime
- VRChat のゲームデータおよび他の Steam ゲーム

## アンインストール

先に VRChat を終了し、解像度設定を復元できるよう SteamVR は起動したままにしてください。

```sh
# 公式版に戻した上で、Proton コピー・プライベート FEX・ツール本体を削除（Steam が 1 回再起動します）
frame-vrc uninstall

# 完全削除：~/.local/share/frame-vrc と ~/fex-eac-work のキャッシュも削除
frame-vrc uninstall --purge
```

※ Proton Experimental、EAC Runtime、VRChat 本体は Steam に残ります。VRChat の Wine プレフィックス（`steamapps/compatdata/438100`）は Steam 側で管理されており、Steam から VRChat をアンインストールすると削除されます。

### 手動での復元手順

コマンドを使わずに手動で戻す場合：
1. Steam の **VRChat > プロパティ > 互換性** で **デフォルト（Lepton）** を選択します。
2. **VRChat > プロパティ > 一般** で起動オプションを空にします。
3. 上記「作成されるファイルとディレクトリ」に記載されたディレクトリを削除します。
