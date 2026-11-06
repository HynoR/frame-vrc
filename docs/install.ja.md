# インストール

[English](install.md) · [中文](install.zh-CN.md) · **日本語**

すべて Frame 上（Konsole か SSH）で、通常ユーザー（`steamos`）として実行します。sudo は使いません。

## 1. 前提条件

1. Steam で **Proton Experimental** と **Proton EasyAntiCheat Runtime** をインストールします（**FEX-Emu** がなければそれも：ライブラリ > ツール）。
   frame-vrc を入れた後なら、`frame-vrc install-deps` が足りないものごとに Steam のインストール確認を開きます。
2. **VRChat > プロパティ > 互換性 > Proton Experimental。** これをしないと Steam は Android 版を入れます。Windows 版をダウンロードさせてください。
3. SteamVR を一度起動しておきます（frame-vrc は描画解像度を SteamVR 経由で設定します）。最初のセットアップ中は電源につないでください。

## 2. frame-vrc のインストール

```sh
# a) GitHub から：最新リリースをインストール（SHA256SUMS を自分で確認）
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash

# b) ダウンロード済みの frame-vrc-<バージョン>.tar.gz から：確認してからインストール
sha256sum -c --ignore-missing SHA256SUMS        # tarball と SHA256SUMS のあるフォルダで
tar xzf frame-vrc-<バージョン>.tar.gz && sh frame-vrc-<バージョン>/install.sh
```

`~/.local/lib/frame-vrc` に入り、`~/.local/bin/frame-vrc` としてリンクされます（`frame-vrc` が見つからなければ新しいシェルを開いてください）。

## 3. セットアップ

```sh
frame-vrc setup             # 推奨：パッチ版 FEX、DFR 版 Proton、1920 + DFR、検証済みの設定
frame-vrc setup --no-dfr    # DFR なし：標準の Proton Experimental
frame-vrc launch            # または Steam から VRChat を VR で起動
```

Steam に足りないものがあると `setup` は止まって何をすべきか示します。それを済ませてもう一度実行してください。最新リリースから 2 つのパッケージを
ダウンロードし、それぞれ `.sha256` で確認します：

| パッケージ | 中身 | ダウンロードしない場合 |
|---|---|---|
| `fex-eac-<バージョン>-steamframe-aarch64.tar.gz` | パッチ版 FEX | `frame-vrc setup ファイル` か `frame-vrc fetch ファイル`。ヘッドセットでビルドするなら `frame-vrc build`（約 5 分、高温になり、100 °C で自動停止） |
| `dfr-dxvk-<コミット>-steamframe.tar.gz` | DFR 版 DXVK（`d3d11.dll`、`dxgi.dll`）、視線ブリッジとその OpenVR ライブラリ | `frame-vrc setup --dfr ファイル` か `frame-vrc proton dfr ファイル`。自分でビルドする場合は下へ |

ほかから入手したパッケージは、その `.sha256` を隣に置いてください。

最初の `proton dfr` は Proton Experimental を `~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr`（約 1.5 GB）に複製し、DXVK を入れ替え、
複製を認識させるため Steam を 1 回再起動します。Steam が Proton Experimental を更新すると `doctor` が知らせるので、`frame-vrc proton dfr` で複製し直します。

### DFR パッケージを自分でビルドする

[ji10me/dxvk-steam-frame-dfr](https://github.com/ji10me/dxvk-steam-frame-dfr) の手順でビルドし（DXVK は MinGW-w64：
`meson setup build-frame --cross-file build-win64.txt --buildtype release -Denable_d3d8=false -Denable_d3d9=false -Denable_d3d10=false`。
ブリッジは Linux ARM64 で OpenVR SDK `0924064` に対してビルドし、`-Wl,-rpath,'$ORIGIN'` でリンク）、このリポジトリの
`tools/make-dfr-package.sh DXVKのソース OPENVR_SDK` でパッケージにします。

## 4. DFR の動き方

DFR 版 Proton では、VRChat の起動オプションが `…/lib/dfr-run` で始まります。これはゲームより先にネイティブで動き、選んだレベル（`frame-vrc dfr`。
`auto` = 解像度 1600 以上でオン）に合わせて DXVK の DFR 変数を設定し、視線ブリッジを起動します。ブリッジは VRChat が VR シーンを持ってから動き始め、
ゲームと一緒に終わります（ログ：`~/.local/share/frame-vrc/bridge.log`）。ヘッドセットを外していると視線データは無効で、DXVK は全面フルレートで描画します。

## 5. 手動で（frame-vrc なし）

1. FEX パッケージを展開し（`~/fex-eac-work/stage`）、すべてのゲームを終了して（`pgrep -x FEXServer` が何も出さない）、`sh lib/install-fex-tool.sh` を実行
   （`restore` で Valve のファイルに戻ります）。
2. 起動オプションを貼り付けます（ツールのある機械で `frame-vrc launch-options show` が表示します）。ji10me の手順で DFR を用意していなければ `…/dfr-run` は外します。
3. 解像度：`/opt/steamvr/bin/linuxarm64/vrcmd --set-settings-int steam.app.438100.resolutionOverride 1920`
   （先に `--settings-int steam.app.438100.resolutionOverride` で元の値を控えておく）。

## 6. メモリを節約する VRChat の設定

VRChat（VR 内の設定）で：Poor / Very Poor のアバターを非表示、アバターのダウンロードと展開サイズの上限を下げる、表示するアバターの最大数を下げる、
パーティクル制限をオン、ミラーは低品質かオフ。遊ぶ前にヘッドセットを再起動すると Steam の画面のプロセスがリセットされます。

## 7. 動作確認

`frame-vrc doctor` の最後が `all set` になるはずです。記録するには：先に `frame-vrc bench` を実行してからゲームを起動します。
`~/.local/share/frame-vrc/bench/` のレポートに EAC が起動したか（`Launcher finished with: 301`、`AntiCheat Session Begin: Success`）が出ます。
ゲームが起動しないときは、起動オプションに一度だけ `PROTON_LOG=1` を加えて `~/steam-438100.log` を見てください。

すべて戻す：[changes.ja.md](changes.ja.md)。
