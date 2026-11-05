# frame-vrc

[English](README.md) · [中文](README.zh-CN.md) · **日本語**

> **いじるのが好きな技術者向けです。** これは非公式の研究プロジェクトで、Linux のコマンドラインを使え、ログを読み、
> 問題が起きたら自分で元に戻せる人を対象にしています。そうでない方は公式対応を待ってください。初心者の方は安易に試さないでください。
> 自己責任でお願いします。[免責事項](#免責事項)を参照。

**Steam Frame** 単体で **PC（Windows）版 VRChat** を Easy Anti-Cheat 込みで動かします。[StonkGuy/FEX](https://github.com/StonkGuy/FEX)
のパッチ版 FEX（`eac-*` タグ、[steamac](https://github.com/StonkGuy/steamac) の `fex-eac/` 由来）を使います。一つのツールで環境チェック、
バックアップ付きの FEX ビルド切り替え、起動オプションと描画解像度の設定、比較できるベンチマークの記録を行います。

EAC は回避していません。Valve の Proton EasyAntiCheat Runtime に入っている純正の EAC クライアントをエミュレーション上で動かしており、
パッチ版 FEX はそのためにエミュレーションを十分正確にするだけです。

目的は **VRChat を遊ぶこと**です。EAC を使うほかのゲームは対象外で、テストもしていません。パッチ版 FEX で動くとは限らず、
ここの設定も VRChat 向けに調整しています。

**研究段階**、検証済みのヘッドセットは 1 台（SteamOS 0.4.5 vr、2026 年 11 月）。

## 動作状況

| FEX ビルド（`frame-vrc switch …`） | VR | デスクトップ（`--no-vr`） |
|---|---|---|
| `valve`：Steam 標準の FEX-Emu | EAC が失敗（パッチの理由。steamac の Mac 環境で確認） | 同左 |
| `eac`：steamac のパッチ版 FEX | **遊べる**：EAC 正常、ワールドに入れる。1440 で約 20 fps、1056 で約 29 fps、720 で約 36 fps | 遊べる |
| `thunk`：`eac` + Vulkan thunk | 最初のフレームを SteamVR に渡すときにクラッシュ（FEX/Valve に任せる） | 遊べる、CPU 約 10% 減 |

ボトルネックはメモリです。SteamVR、Steam の UI、トラッキングサービスで 16 GB 中約 7 GB を使い、人の多いワールドでは VRChat が残りと zram を
使い切ります。[わかったこと](docs/history.ja.md#わかったこと)を参照。

## クイックスタート

Frame 上で（Konsole または SSH）、通常ユーザーで（sudo は使わない）：

```sh
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash   # frame-vrc をインストール
~/.local/bin/frame-vrc setup --resolution 1056
```

`setup` はすべてを順番に行います：FEX-Emu、Proton Experimental、EAC Runtime がなければ Steam にインストールを依頼し、VRChat が
Proton Experimental 上の Windows 版か確認し、最新リリースからパッチ版 FEX をダウンロード・検証し、入れ替え（Valve のファイルはバックアップ）、
VRChat の起動オプションと描画解像度を設定し、最後に `doctor` を実行します。途中で止まった場合は Steam でやるべきことが表示されるので、
それをしてからもう一度実行してください。`--resolution` を付けなければ SteamVR の設定はそのまま。普段は 1056、720 は特に厳しいときだけ（約 36 fps、かなりぼやける）。
その後 VR で VRChat を起動します。最初のワールド入場は約 3 分です。

その他の方法（Discord で配布されたパッケージ、ヘッドセット上でのビルド、完全手動）：[docs/install.ja.md](docs/install.ja.md)。

## ドキュメント

| | |
|---|---|
| [インストール](docs/install.ja.md) | 前提条件、ビルド済み／自前ビルド／完全手動のインストール、VRChat の設定、動作確認 |
| [変更内容と元に戻す方法](docs/changes.ja.md) | 触るファイルと設定のすべて、その場所、戻し方、完全アンインストール |
| [経緯とわかったこと](docs/history.ja.md) | 試したこと・測ったこと・わかったことを時系列で。未解決の課題 |
| [Vulkan thunk のビルド](docs/thunk-build.ja.md) | 任意の `thunk` ビルド |
| [実験記録](docs/research/) | 生の記録（中国語） |

## コマンド

```text
frame-vrc setup [--resolution N] [URL|FILE]  以下をまとめて実行（指定がなければ最新リリースの FEX）
frame-vrc doctor [game]                    すべてを確認し、直すべき点を表示
frame-vrc install-deps [game]              FEX-Emu、Proton Experimental、EAC Runtime のインストールを Steam に依頼
frame-vrc build [--jobs N]                 ヘッドセット上でパッチ版 FEX をビルド（約 5 分、かなり熱くなる、100°C で停止）
frame-vrc fetch URL|FILE                   配布ビルドを使う（.sha256 を隣に置く）：検証して stage に置く
frame-vrc pack [eac|thunk]                 自分のビルドを他の人向けのパッケージにする
frame-vrc switch status|valve|eac|thunk    Steam の FEX-Emu を切り替え（先に Valve のファイルをバックアップ）
frame-vrc launch-options show|set|clear    VRChat の起動オプション（Steam 自身の API 経由）
frame-vrc resolution show|N|reset          SteamVR での VRChat の描画解像度。設定画面の下限 1440 より下げられる
frame-vrc bench [game]                     1 セッションの CPU、メモリ、温度、SteamVR の fps、EAC の結果を記録
frame-vrc uninstall [--purge]              すべて元に戻す
```

## 安全性

通常ユーザーで動き、sudo は使わず、`~` の下にしか書き込みません。システム、ファームウェア、`/persist` は変更しません。
すべての変更は元に戻せます（[docs/changes.ja.md](docs/changes.ja.md)）。影響するのは Steam が FEX-Emu 互換ツール（x86 Proton）で動かすゲームだけです。

## 研究への協力

`frame-vrc bench` のレポート（`~/.local/share/frame-vrc/bench/`）を **Benchmark report** の issue テンプレートか Discord で共有してください。
同じワールド、同じ時間、一度に変えるのは一つだけにして、何を変えたかを書いてください。自動アップロードはしません。

## 謝辞と関連プロジェクト

- [steamac](https://github.com/StonkGuy/steamac) と [StonkGuy/FEX](https://github.com/StonkGuy/FEX)（FX GAMES）：Proton で EAC を動かせるようにしたパッチ版 FEX と、
  このツールの出発点になった `fex-eac/` のビルド・インストールスクリプト。
- [FEX-Emu](https://github.com/FEX-Emu/FEX)：ここにあるすべてが動いている x86 エミュレータ。
- Valve：Proton、Proton EasyAntiCheat Runtime、SteamVR、Steam Frame 用の FEX-Emu ツール。
- [DXVK](https://github.com/doitsujin/dxvk) と Mesa の Turnip ドライバ：Frame の GPU で Direct3D と Vulkan を動かす。
- [Decky Loader](https://github.com/SteamDeckHomebrew/decky-loader)：起動オプションの設定に使っている Steam CEF の方法。
- X の [@ji10me](https://x.com/ji10me) さん、Steam Frame での VRChat についての発信に感謝します：
  https://x.com/ji10me/status/2117211791570919632 · https://x.com/ji10me/status/2116820842579628327

## 免責事項

- **初心者向けではありません。** 画質は粗く、fps は低く、初回のロードは長く、クラッシュすることもあり、ログを読む必要があります。
  問題が起きたら [docs/changes.ja.md](docs/changes.ja.md) を見て自分で元に戻せることが前提です。
- **非公式です。** Valve、VRChat、Epic Games（Easy Anti-Cheat）、FEX-Emu、FX GAMES とは無関係で、承認も受けていません。いずれも Frame での
  PC 版の動作をサポートしておらず、Steam、SteamVR、Proton、VRChat のアップデートでいつでも動かなくなる可能性があります。
- **アカウントについて。** EAC を回避・改変はしていませんが、この動かし方を受け入れるかどうかは VRChat と EAC の判断です。ゲームファイルは
  改変しないでください。VRChat の利用規約を守るのはご自身の責任です。
- **ハードウェアについて。** ビルド中やプレイ中はヘッドセットがかなり熱くなります（実測で最大約 98°C）。電源につなぎ、風通しを良くし、
  不快に感じたら中止してください。
- **無保証です。** Apache-2.0 ライセンスのもと「現状のまま」提供され、いかなる保証もありません。作者は損害、データの消失、アカウントへの
  措置について一切責任を負いません。

## ライセンス

Apache-2.0。[LICENSE](LICENSE) と [NOTICE](NOTICE) を参照。
