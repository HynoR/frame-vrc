# frame-vrc

[English](README.md) · [中文](README.zh-CN.md) · **日本語**

> [!WARNING]
> **免責事項（最初にお読みください）。** frame-vrc は非公式のプロジェクトで、Valve、VRChat、Epic Games が作成・サポートする
> ものではありません。非公式で改変された多くのコンポーネントをインストール・切り替えします：パッチ版 FEX（x86 エミュレーター）、
> 改変版 DXVK（注視点レンダリング）を入れた Proton Experimental の複製、Steam クライアント経由で変更する Steam の設定。
> 使うには Linux、コマンドライン、ログの読み方についてある程度の知識と、問題が起きたときに自分で元に戻せることが必要です。
> **よく分からない場合は、実行する前に慎重に考えてください**。または公式のサポートを待ってください。自己責任でお使いください
> （全文は[免責事項](#免責事項)）。

**Steam Frame** 単体で **PC（Windows）版 VRChat** を Easy Anti-Cheat 込みで動かします。[StonkGuy/FEX](https://github.com/StonkGuy/FEX) の
パッチ版 FEX（`eac-*` タグ、[steamac](https://github.com/StonkGuy/steamac) の `fex-eac/` 由来）を使い、
フォービエイテッドレンダリング（DFR、[ji10me/dxvk-steam-frame-dfr](https://github.com/ji10me/dxvk-steam-frame-dfr)）も選べます。
ひとつのツールで公式版とパッチ版の切り替え、検証済みの設定、ゲームの起動、すべての復元ができます。

EAC は回避していません。Valve の Proton EasyAntiCheat Runtime に入っている標準の EAC クライアントがエミュレーション上で動き、
パッチ版 FEX はそのためにエミュレーションを十分正確にしているだけです。目的は **VRChat を遊ぶこと**で、ほかの EAC ゲームは未検証です。

**研究段階**、ヘッドセット 1 台で確認済み（SteamOS 0.4.5 vr、2026 年 11 月）。

## クイックスタート

1. Steam で **Proton Experimental** と **Proton EasyAntiCheat Runtime** をインストールし、**VRChat > プロパティ > 互換性 >
   Proton Experimental** にして Windows 版をダウンロードさせます。**FEX-Emu** がなければそれも入れます（ライブラリ > ツール）。
2. Frame 上で frame-vrc を 1 行でインストールします（Konsole か SSH、通常ユーザー、sudo は使わない）。最新リリースを
   ダウンロードして SHA-256 を確認し、VR パネルをビルドします（約 1 分）：

```sh
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash
```

3. ヘッドセットで：SteamVR ダッシュボード > ライブラリ（アプリ一覧）> **frame-vrc** > **まとめて設定**（2 回タップ）、
   続けて **VRChat を起動**。コマンドラインなら `~/.local/bin/frame-vrc setup` と `~/.local/bin/frame-vrc launch`。

設定は推奨構成を適用します：パッチ版 FEX、DFR 版 Proton、描画解像度 1920 + DFR、下の検証済みの設定。2 つのパッケージは
最新リリースからダウンロードし、SHA-256 を確認します。**Steam と SteamVR が 1 回再起動します**（新しい Proton を認識させるため）。
画面が 1 分ほど消えてから戻り、設定はそのまま続きます。各ステップはログページで見られます。最初のワールド参加は約 3 分、以降は約 20 秒です。

**VR パネル**は現在の設定を表示し、環境（VRChat、Proton Experimental、EAC Runtime、FEX-Emu、起動オプション）を確認して
問題ごとに修正ボタンを出します。解像度、DFR、詳細スイッチ、モニターページ（メモリ、スワップ、コアごとの CPU、温度、
VRChat のメモリと CPU）、ログページもあります。表示は英語か日本語（サイドバーのボタン）。設定のボタンは最後のタップから
1.5 秒後にまとめて適用されます。常駐はせず、ダッシュボードの閉じるボタンで終了します。どのボタンも `frame-vrc` のコマンドを 1 つ実行します。

公式版に戻す：`frame-vrc uninstall`。詳細とほかの方法：[docs/install.ja.md](docs/install.ja.md)。

## スイッチ

| コマンド | 選択肢 | 既定 |
|---|---|---|
| `frame-vrc fex` | `valve` Steam の FEX · `eac` パッチ版（EAC に必要） | eac |
| `frame-vrc proton` | `stock` Proton Experimental · `dfr` その複製に DFR 版 DXVK を入れたもの（Steam の本体はそのまま） | dfr |
| `frame-vrc resolution` | `1056` 性能 · `1440` バランス · `1920` 画質；上級：`1600`、`2160`、任意の N | 1920 |
| `frame-vrc dfr` | `auto` 1440 以上で自動オン · `off` · `def`；上級：`wide`、`ext`（2160 以上向け） | auto |
| `frame-vrc tune`（上級） | `cpus N` · `dxvk-threads N` · `vram MB` · `reset` | 16 · 4 · 1024 |

中間の解像度は SteamVR の設定で調整できます。1056 を載せているのは SteamVR の画面では 1440 までしか下げられないためです。
`frame-vrc status` で現在の設定をすべて表示し、`frame-vrc doctor` ですべてを確認して直し方を示します。

常に有効（VR で測定済み、スイッチにするほどの欠点がないもの）：FEX のコードキャッシュは VRChat だけ（Wine の補助プロセス −640 MB）、
FEX の翻訳ブロック 500 命令（ゲームのメモリ −0.65 GB、zram −1.9 GB）、Wine のタッチキーボードをオフ（−135 MB）、デスクトップのミラーウィンドウ 640×480。

## 効果があるもの、ないもの

- **効果あり：** パッチ版 FEX（ないと EAC が起動しない）、上の設定、**1920 での DFR**：重いワールドでフレームレート +15〜25%
  （シェーダーが重いほど効果が大きく、軽いワールドではノイズの範囲）。DFR が節約するのは GPU 時間で、メモリは減りません。
- **効果なし：** Vulkan thunk（VR で落ちる）、FEX の TSO オフ（落ちる）、1440 での DFR、より粗い DFR（1920 で `ext` は `def` より速くない）、
  1440 未満の解像度（非常時のみ）。

限界はメモリです：SteamVR、Steam の画面、トラッキングで 16 GB のうち約 7 GB を使います。重いワールドに 1 人、1920 で約 1.5 GB が残ります。
数値：[docs/research/summary.zh-CN.md](docs/research/summary.zh-CN.md)（中国語）。

## コマンド

```text
frame-vrc setup [--no-dfr] [--resolution N] [--dfr URL|FILE] [FEX の URL|FILE]
frame-vrc panel | status | doctor | launch | update
frame-vrc fex valve|eac             frame-vrc proton stock|dfr [URL|FILE]
frame-vrc resolution 1056|1440|1920 frame-vrc dfr auto|off|def
frame-vrc uninstall [--purge]       Valve の FEX、Proton Experimental、元の起動オプションと解像度に戻す
上級：resolution N · dfr wide|ext · tune · install-deps · build · fetch · pack · launch-options · bench
```

## ドキュメント

| | |
|---|---|
| [インストール](docs/install.ja.md) | 前提条件、パッケージ・自分でビルド・手動のインストール、動作確認 |
| [何が変わるか、戻し方](docs/changes.ja.md) | 触るファイルと設定のすべて、戻し方、完全アンインストール |
| [履歴](docs/history.ja.md) | バージョンごとに試したことと分かったこと |
| [研究のまとめ](docs/research/summary.zh-CN.md) | 測定結果（中国語） |

## 安全性

通常ユーザーで動き、sudo は使わず、`~` 以下にだけ書き込みます。システム、ファームウェア、`/persist` は変更しません。
すべての変更は元に戻せます（[docs/changes.ja.md](docs/changes.ja.md)）。変わるのは VRChat の設定と Steam の FEX-Emu ツールだけで、
Proton Experimental 本体は一切変更しません（DFR は複製を使います）。

## 研究への協力

`frame-vrc bench` のレポート（`~/.local/share/frame-vrc/bench/`）を **Benchmark report** の issue テンプレートか Discord で共有してください。
同じワールド・同じ長さで、一度に 1 つだけ変え、何を変えたかを書いてください。自動アップロードは一切ありません。

## 謝辞と関連プロジェクト

- [steamac](https://github.com/StonkGuy/steamac) と [StonkGuy/FEX](https://github.com/StonkGuy/FEX)（FX GAMES）：Proton で EAC を動かすパッチ版 FEX と、
  このツールの出発点になった `fex-eac/` のビルド・インストールスクリプト。
- [ji10me/dxvk-steam-frame-dfr](https://github.com/ji10me/dxvk-steam-frame-dfr)（[@ji10me](https://x.com/ji10me)）、ReinaS-64892 と poprox24 の
  DFR の成果に基づく：フォービエイテッドレンダリングの DXVK と視線ブリッジ。
- [FEX-Emu](https://github.com/FEX-Emu/FEX)：ここのすべてが動いている x86 エミュレーター。
- Valve：Proton、Proton EasyAntiCheat Runtime、SteamVR、OpenVR、Steam Frame の FEX-Emu ツール。
- [DXVK](https://github.com/doitsujin/dxvk) と Mesa の Turnip ドライバー：Frame の GPU 上の Direct3D と Vulkan。
- [Decky Loader](https://github.com/SteamDeckHomebrew/decky-loader)：起動オプションの設定に使う Steam CEF の方法。

## 免責事項

- **初心者向けではありません。** 画面がぼやける、フレームレートが低い、最初の読み込みが長い、落ちる、ログを読む必要がある、と考えてください。
  問題が起きたら [docs/changes.ja.md](docs/changes.ja.md) に従って自分で戻せる必要があります。
- **非公式です。** Valve、VRChat、Epic Games（Easy Anti-Cheat）、FEX-Emu、FX GAMES とは無関係で、承認も受けていません。Frame で PC 版を動かすことは
  どこもサポートしておらず、Steam、SteamVR、Proton、VRChat の更新でいつでも動かなくなる可能性があります。
- **あなたのアカウント。** EAC は回避も改変もしていませんが、この環境を受け入れるかどうかは VRChat と EAC が決めます。ゲームファイルは改変しないでください。
  VRChat の利用規約を守る責任はあなたにあります。
- **あなたのハードウェア。** ビルドとプレイでヘッドセットは高温になります（実測で最大約 98 °C）。電源につなぎ、風通しを良くし、不快なら止めてください。
- **無保証。** Apache-2.0 ライセンスの下で「現状のまま」提供され、いかなる保証もありません。作者は損害、データの消失、アカウントへの処分について責任を負いません。

## ライセンス

Apache-2.0、[LICENSE](LICENSE) と [NOTICE](NOTICE) を参照。DFR パッケージには独自のライセンス（DXVK と Frame 部分：zlib、GAZ2 ヘッダー：MIT、
OpenVR：BSD-3-Clause）が `dfr/licenses/` に入っています。
