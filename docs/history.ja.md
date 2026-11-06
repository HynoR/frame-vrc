# 履歴

[English](history.md) · [中文](history.zh-CN.md) · **日本語**

Steam Frame 1 台（SteamOS 0.4.5 vr、Snapdragon 8 Gen 3、16 GB）、VRChat の Windows 版を Proton Experimental で。
測定結果：[research/summary.zh-CN.md](research/summary.zh-CN.md)（中国語）。

## バージョン

| バージョン | 日付 | 変更点 |
|---|---|---|
| 0.6.1 | 2026-11-06 | 新規インストールでの DFR：視線ブリッジのフォルダを先に作る（ないとブリッジが視線データを書けず、DFR がずっとフルレートのままだった）。 |
| 0.6.0 | 2026-11-06 | フォービエイテッドレンダリング（DFR）：`proton stock\|dfr`（Proton Experimental の複製に ji10me の DFR 版 DXVK）、`dfr auto\|off\|def`（auto = 1600 以上でオン）、ゲームと一緒に起動する視線ブリッジ（`lib/dfr-run`）。推奨構成：1920 + DFR。スイッチを整理（`fex`、`proton`、`resolution`、`dfr`、`tune`）、設定は 1 ファイルに、`status` と `launch` を追加。デスクトップのミラーウィンドウは既定で 640×480。Vulkan thunk ビルドを削除。 |
| 0.5.3 | 2026-11-05 | FEX のコードキャッシュを VRChat と EAC ランチャーだけに：Wine の補助プロセスが約 640 MB 減、フレームレートは変わらず。 |
| 0.5.2 | 2026-11-05 | `FEX_MAXINST=500` を既定に（ゲームのメモリ −0.65 GB、zram −1.9 GB、フレームレート約 −10%）。クリーンインストールのテストで見つかった修正。 |
| 0.5.0–0.5.1 | 2026-11-05 | `setup` 1 コマンドでインストール。1 行インストーラー（`get.sh`）。 |
| 0.4.0 | 2026-11-05 | frame-eac から改名：目的は PC 版 VRChat で、EAC ゲーム全般ではない。 |
| 0.3.0 | 2026-11-05 | DXVK の VRAM 報告 1 GB + コンパイルスレッド 4（CPU −15%）、tabtip オフ（−135 MB）、`resolution`、ベンチマークに SteamVR のフレームレート、3 言語のドキュメント。 |
| 0.1–0.2 | 2026-11-03 | 最初の実行：steamac のパッチ版 FEX をヘッドセットでビルド。EAC `301` / `Session Begin: Success`。VRChat が VR で遊べる。 |

## 要点

- **効果あり：** パッチ版 FEX（ないと EAC が起動しない）、上の既定の設定、1920 での DFR（シェーダーが重いワールドでフレームレート +15〜25%、軽いワールドではノイズの範囲。
  GPU 時間は減るがメモリは減らない）。DFR の DLL は Proton 自身の DXVK フォルダに置く必要があり、`VRChat.exe` の隣に置くと Wine のローダーで落ちます。
- **効果なし：** Vulkan thunk（VR で SteamVR の x86 クライアントがフレームを渡すと落ちる。デスクトップのみで CPU −10%）、FEX の TSO オフ（約 35 秒で落ちる）、
  HalfBarrier TSO オフ（効果なし）、Mesa の VRAM 上限（ネイティブ側のみ）、1440 での DFR（効果なし）、より粗い DFR（1920 で `ext` は `def` より速くない）。
- **限界：** メモリ（SteamVR、Steam の画面、トラッキングで 16 GB のうち約 7 GB）と CPU（FEX による x86 の翻訳：1056 で 1 フレーム約 20 ms、
  重いワールドの 1920 で約 35〜40 ms）。SteamVR はゲームを 72 Hz の半分か 3 分の 1 で動かします。
- **測定：** ゲームの `/proc/<pid>/smaps` 全体を読むとカクつく（サンプラーは `smaps_rollup` を使う）。無人で測るときはヘッドセットのスリープを切る
  （スタンバイで描画が止まり、ログインも止まる）。

## 未解決

1. 1920 で人の多いワールドのメモリ（重いワールドに 1 人で約 1.5 GB 残る）。
2. ワールドの動画プレイヤーのハードウェアデコード（現在はエミュレーションのソフトウェアデコード）。
3. ヘッドセットの再起動で Steam の画面のプロセスがどれだけ減るか。
