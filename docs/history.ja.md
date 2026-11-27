# リリース履歴

[English](history.md) · [中文](history.zh-CN.md) · **日本語**

| バージョン | 日付 | 変更内容 |
|---|---|---|
| 0.8.1-rc1 | 2026-11-27 | プレリリース。セットアップをパネル内の 8 ステップのウィザードに変更：FEX-Emu、Proton Experimental、EasyAntiCheat ランタイムの Steam インストール画面を 1 つずつ開くようにしました（以前は SteamVR に最後の 1 つしか表示されませんでした）。インストール画面の再表示、再試行、中断後の再開に対応。[ji10me 氏のランタイム 0.3.7](https://github.com/ji10me/SteamFrame_Standalone_PCVR_VRChat) に同期：fsync/esync/ntsync を無効にし、ログイン時やワールド参加時に VRChat が止まる問題を修正。2026-11-10 の Proton Experimental 更新後も音声修正が有効。 |
| 0.8.0 | 2026-11-09 | 初回公開リリース。[ji10me 氏の PCVR ランタイム 0.3.6](https://github.com/ji10me/SteamFrame_Standalone_PCVR_VRChat)（視線追跡 DFR、H.264 ハードウェア動画デコード、Wine 音声修正、Vulkan thunk、VRChat 用スレッド配置）を統合。ヘッドセット単体でのインストール、VRChat 専用プライベート FEX 環境、VR パネルからの公式版／PCVR 版ワンタップ切り替え、DFR 動作確認コマンド `dfrtest` に対応。 |
| 0.1–0.7 | 2026-11-03 – 11-06 | プレリリース：EAC 対応パッチ版 FEX、DFR、VR パネルのプロトタイプ実装。 |
