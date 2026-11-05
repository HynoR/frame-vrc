# 経緯とわかったこと

[English](history.md) · [中文](history.zh-CN.md) · **日本語**

Steam Frame 1 台（SteamOS 0.4.5 vr、Snapdragon 8 Gen 3、16 GB）、VRChat Windows 版、Proton Experimental。生の記録：[research/](research/)（中国語）。

## 時系列

### 2026-11-03：EAC 付きの VRChat が VR で動いた

- steamac の `fex-eac`（Proton 上で EAC を動かすためのパッチ版 FEX、Mac で実証済み）をヘッドセット上でビルド：4 ジョブで約 5 分、約 2 GB、
  最大約 98°C。Valve のファイルをバックアップしてから Steam の FEX-Emu に入れ替え。すべての手順に戻し方を用意。
- 起動オプションは VR 内で入力せず、Steam 自身の API（CEF デバッグポート）で設定。
- 結果：EAC `Launcher finished with: 301`、`AntiCheat Session Begin: Success`、**VR で**ワールドに入場。最初の入場は約 3 分（FEX のコード
  キャッシュが空）、以降は約 20 秒。
- ベースライン（VR、1440）：ゲーム CPU 約 280%（100% = 1 コア）、ゲームのメモリ（PSS）平均 4.4 GB／最大 5.4 GB、GPU メモリ約 2.2 GB、
  空きメモリ最低約 0.7 GB、zram 満杯、約 94°C。
- Vulkan thunk（エミュレーションの代わりにネイティブの Vulkan）：Valve のビルド済みも eac 版 FEX からビルドしたものも、ロード中に止まった。
- `WINE_CPU_TOPOLOGY` 16 対 8：はっきりした差なし（ワールドが違う）。
- frame-vrc 0.1–0.2：Valve / eac / thunk ビルドの切り替え、起動オプション、ベンチマーク。

### 2026-11-05：thunk の診断、デスクトップモード、VR のメモリと fps

- **thunk の原因 1、修正済み：** ネイティブの Vulkan ローダーが、コンテナが x86 向けに用意したドライバ一覧（エミュレーション側からしか
  見えないパス）を受け取り、ドライバが見つからなかった。`FEX_HOSTENV=VK_DRIVER_FILES=<json>` でホストの Turnip を指定。
- **thunk の原因 2、FEX/Valve に任せる：** VR では SteamVR の x86 vrclient が thunk 経由で最初のフレームを渡すときにゲームがクラッシュ。
  修正には FEX の thunk ABI のコード変更が必要。
- **デスクトップモード**は thunk の有無にかかわらず動作、描画正常。thunk で CPU 約 10% 減、メモリは変わらず。
- **VR のメモリと fps**（ホームワールド、一人、各 5 分、自動化）：

  | 回 | 変更 | fps | ゲーム GPU メモリ | zram 最大 | 空きメモリ最低 |
  |---|---|---|---|---|---|
  | A–C | ベースライン（VRAM 上限の実験。実際はどれも無効だった） | 約 20 | 約 2.4 GB | 約 7.5 GB | 約 0.8 GB |
  | D | DXVK で VRAM 1 GB と報告 + コンパイル 4 スレッド、tabtip 無効 | 約 20、CPU −15% | 2.4 GB | 7.4 GB | 1.1 GB |
  | E | D + 解像度 1056 | 29 | 2.1 GB | 7.2 GB | 0.9 GB |
  | F | D + 解像度 720 | 36（SteamVR の統計） | 1.8 GB | 6.5 GB | 1.1 GB |

- frame-eac から **frame-vrc** に改名（0.4.0）：目的は PC 版 VRChat を遊ぶことで、EAC のゲーム全般ではない。
- frame-vrc 0.3.0：D の設定をデフォルトに、`resolution` コマンド、ベンチマークに SteamVR の fps、thunk ビルドのパッケージ化、
  `fetch` でローカルファイル、3 言語のドキュメント。

## わかったこと

- **描画解像度が一番効く。** SteamVR の設定画面は 1440 が下限だが、`vrcmd`（と `frame-vrc resolution`）ならさらに下げられる。SteamVR は
  32 の倍数に丸める（1080 → 1056）。
- **CPU も GPU も限界に近い。** 1056 で 1 フレームあたり CPU 約 20 ms、GPU 約 15 ms。SteamVR はゲームを 36 または 24 fps で刻む（36 には
  27.7 ms 以内が必要）。CPU 時間の大半は FEX による x86 コードの変換。
- **ゲームから見える VRAM：** Steam の Mesa による VRAM 上限はネイティブ側でしか効かず、`eac` では Unity に実際の 14683 MB が見える。
  DXVK の `dxgi.maxDeviceMemory` は効く。
- **DXVK のコンパイルスレッド**は報告された CPU 数（16）に従う。4 にすると CPU が約 15% 減。
- **フォービエイテッドレンダリング：** SteamOS の `VK_LAYER_VALVE_fdm_injection` は aarch64 版しかないため、エミュレーションで動くゲームには
  効かない。thunk なら読み込まれる。VR の thunk クラッシュが重要なのはこのため。
- **メモリの使い道：** SteamVR だけの状態で 16 GB 中約 8.7 GB が空き。Steam の UI（約 1.5 GB + zram に 1.4 GB）、SteamVR ダッシュボード
  （約 0.7 GB）、XRService（約 0.7 GB）は VR で必要。ゲーム中は Wine の補助プロセスがそれぞれ約 135 MB の FEX アロケータを抱え、合計約 1.2 GB。
- **測定の注意：** ゲームの `/proc/<pid>/smaps` 全体を読むとカクつく。frame-vrc のサンプラーは `smaps_rollup` を使う。
  `vrcmd --stats` でゲームのフレーム数と 1 フレームあたりの CPU/GPU 時間がわかる。

## 未解決の課題

1. Wine の補助プロセスごとの約 135 MB の FEX アロケータ：固定コストかアロケータのキャッシュか。小さく元に戻せる修正があれば試す価値あり。
   なければ FEX に報告。
2. 人の多いワールドでの 1 GB VRAM 報告と VRChat のアバター制限の効果。
3. ヘッドセットの再起動で Steam の UI プロセスがどれだけ減るか。
4. VR の thunk クラッシュ（上流）。SteamVR の x86 vrclient の代わりに OpenVR→OpenXR の変換レイヤー（xrizer など）を使えば回避できるかも
   しれない。未検証。EAC があるのでゲームファイルは変更できない。
