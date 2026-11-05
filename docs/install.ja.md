# インストール

[English](install.md) · [中文](install.zh-CN.md) · **日本語**

作業はすべて Frame 上で（Konsole または SSH）、通常ユーザー（`steamos`）で行います。sudo は使わないでください。

## 1. 前提条件（どの方法でも必要）

1. Steam で **FEX-Emu**（ライブラリ > ツール）、**Proton Experimental**、**Proton EasyAntiCheat Runtime** をインストール。
   frame-vrc を入れた後なら、`frame-vrc install-deps` が足りないものごとに Steam のインストール確認を開きます。
2. **VRChat > プロパティ > 互換性 > Proton Experimental。** 設定しないと Android 版が入ります。Windows 版をダウンロードさせてください。
3. 初回のセットアップ中はヘッドセットを電源につないでおいてください。スリープすると SSH が切れます。

## 2. frame-vrc のインストール

どちらか一つ：

```sh
# a) GitHub から：最新リリースをインストール（SHA256SUMS を自動で検証）
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash

# b) ダウンロードした frame-vrc-<version>.tar.gz から（Discord など）：検証してからインストール
sha256sum -c --ignore-missing SHA256SUMS        # tarball と SHA256SUMS があるフォルダで
tar xzf frame-vrc-<version>.tar.gz && sh frame-vrc-<version>/install.sh
```

`~/.local/lib/frame-vrc` に入り、`~/.local/bin/frame-vrc` にリンクされます。`frame-vrc` が見つからない場合は、新しいシェルを開くか
`~/.local/bin/frame-vrc` を使ってください。

## コマンド一つで

frame-vrc をインストールしたら、`frame-vrc setup --resolution 1056` が下の 1、3、4 をまとめて行います（前提条件、最新リリースの FEX パッケージ、
切り替え、起動オプション、解像度、doctor）。別のパッケージを使うには URL かファイルを指定します：
`frame-vrc setup ~/Downloads/fex-<version>-steamframe-aarch64.tar.gz`。以下は同じ手順を一つずつ行う方法です。

## 3. パッチ版 FEX を用意する

### 方法 A：ビルド済みパッケージ（おすすめ）

`fex-<version>-steamframe-aarch64.tar.gz` **と** その `.sha256` を同じフォルダにダウンロードするか、リリースの URL を指定します：

```sh
frame-vrc fetch ~/Downloads/fex-<version>-steamframe-aarch64.tar.gz      # Discord からのファイル
frame-vrc fetch https://github.com/.../fex-<version>-steamframe-aarch64.tar.gz   # リリースのファイル
```

チェックサムが合わない、または中の FEX が動かない場合、`fetch` は拒否します。前のビルドは `stage.prev` として残ります。
`-thunk-` パッケージも同じ使い方です（任意、デスクトップのみ）。

### 方法 B：ヘッドセット上でビルド

```sh
frame-vrc build              # 4 ジョブで約 5 分、~/fex-eac-work に約 2 GB
```

ヘッドセットはかなり熱くなります（実測で最大約 98°C）。100°C で自動停止し、もう一度実行すると続きからビルドします。
電源につなぎ、頭から外しておいてください。

### 方法 C：完全手動（frame-vrc なし）

1. パッケージを展開：`mkdir -p ~/fex-eac-work && sha256sum -c fex-*.sha256 && tar xzf fex-*.tar.gz -C ~/fex-eac-work`
   （`~/fex-eac-work/stage` ができます）。
2. ゲームをすべて終了（FEXServer が動いていないこと：`pgrep -x FEXServer` が何も出力しない）。
3. steamac のスクリプト（このリポジトリの `lib/install-fex-tool.sh` にもあります）で入れ替え。先に Valve のファイルをバックアップします：
   `sh install-fex-tool.sh`（`status` で入っているものを表示、`restore` で Valve のものに戻す）。
4. 下の起動オプションを VRChat > プロパティ > 起動オプション に貼り付け。
5. 任意で解像度：`/opt/steamvr/bin/linuxarm64/vrcmd --settings-int steam.app.438100.resolutionOverride`
   （元の値をメモ）、それから `--set-settings-int steam.app.438100.resolutionOverride 1056`（特に厳しいときは 720）。

## 4. 切り替えと設定

```sh
frame-vrc switch eac             # Valve の FEX をバックアップし、パッチ版を入れる
frame-vrc launch-options set     # Steam 自身の API で設定。`show` なら表示して手で貼れる
frame-vrc resolution 1056        # 任意。720 は特に厳しいときだけ。`reset` で元の値に戻す
frame-vrc doctor
```

`eac` ビルドで frame-vrc が設定する起動オプション：

```text
env DXVK_CONFIG="dxgi.maxDeviceMemory = 1024; dxvk.numCompilerThreads = 4" WINEDLLOVERRIDES="tabtip.exe=d" FEX_MAXINST=500 EAC_LAUNCHERDIR=$HOME/.local/share/Steam/steamapps/compatdata/438100/pfx/drive_c/users/steamuser/AppData/Roaming/EasyAntiCheat PROTON_EAC_RUNTIME="$HOME/.local/share/Steam/steamapps/common/Proton EasyAntiCheat Runtime" WINEDEBUG=-all PROTON_USE_XALIA=0 WINE_CPU_TOPOLOGY=16:0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7 %command%
```

各部分の役割：`EAC_LAUNCHERDIR` / `PROTON_EAC_RUNTIME` は EAC に Valve のランタイムを使わせる。`WINE_CPU_TOPOLOGY` は CPU を 16 個と報告
（VRChat はこの数でスレッドプールの大きさを決める）。`DXVK_CONFIG` は VRAM を 1 GB と報告し、シェーダーのコンパイルを 4 スレッドにする。
`tabtip.exe=d` は Wine のタッチキーボードを無効化（約 135 MB）。
`FEX_MAXINST=500` は FEX の変換ブロックを小さくする、Valve が Proton ARM64 で使っている値（ゲームのメモリ約 −0.65 GB、zram 約 −1.9 GB、fps 約 −10%）。`PROTON_USE_XALIA=0` と `WINEDEBUG=-all` はアクセシビリティとログの処理を止める。

`thunk` ビルドでは `frame-vrc switch thunk` が起動オプションを自動で変えます（Vulkan thunk のスイッチ、ドライバ修正、`--no-vr`）：
[thunk-build.ja.md](thunk-build.ja.md) を参照。

## 5. メモリを節約する VRChat の設定

VRChat（設定、VR 内で）：Poor／Very Poor のアバターを非表示、アバターのダウンロードサイズと展開後サイズの上限を下げる、表示するアバターの
最大数を減らす、パーティクルリミッターを有効、ミラーは低画質かオフ。人の多いワールドでメモリを埋めるのは他の人のアバターです。
遊ぶ前にヘッドセットを再起動すると、Steam の UI のプロセスがリセットされます。

## 6. 動作確認

Steam から VR で VRChat を起動します。最初のワールド入場は約 3 分（FEX がコードを変換してキャッシュする）、以降は約 20 秒。
1 セッションを記録するには、先に `frame-vrc bench` を実行してからゲームを起動します。レポートは `~/.local/share/frame-vrc/bench/` に保存され、
EAC が起動したか（`Launcher finished with: 301`、`AntiCheat Session Begin: Success`）がわかります。

起動しない場合は、起動オプションに一度 `PROTON_LOG=1` を追加して `~/steam-438100.log` を確認してください。

すべて元に戻す：[changes.ja.md](changes.ja.md)。
