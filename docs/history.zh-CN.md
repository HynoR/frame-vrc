# 历史

[English](history.md) · **中文** · [日本語](history.ja.md)

一台 Steam Frame（SteamOS 0.4.5 vr，骁龙 8 Gen 3，16 GB），VRChat Windows 版跑在 Proton Experimental 上。
实测数据：[research/summary.zh-CN.md](research/summary.zh-CN.md)。

## 版本

| 版本 | 日期 | 改动 |
|---|---|---|
| 0.6.1 | 2026-11-06 | 新安装时的 DFR：先创建眼动 bridge 的文件夹（没有它 bridge 写不了注视点数据，DFR 一直是全速着色）。 |
| 0.6.0 | 2026-11-06 | 注视点渲染（DFR）：`proton stock\|dfr`（Proton Experimental 的副本，换上 ji10me 的 DFR 版 DXVK）、`dfr auto\|off\|def`（auto = 1600 起自动开）、眼动 bridge 随游戏启动（`lib/dfr-run`）。推荐配置：1920 + DFR。开关简化（`fex`、`proton`、`resolution`、`dfr`、`tune`），设置存在一个文件里，新增 `status` 和 `launch`。桌面镜像窗口默认 640×480。删掉 Vulkan thunk 版本。 |
| 0.5.3 | 2026-11-05 | FEX 代码缓存只给 VRChat 和 EAC 启动器：Wine 辅助进程少约 640 MB，帧数不变。 |
| 0.5.2 | 2026-11-05 | 默认 `FEX_MAXINST=500`（游戏内存 −0.65 GB、zram −1.9 GB、帧数约 −10%）。干净安装测试发现的修复。 |
| 0.5.0–0.5.1 | 2026-11-05 | `setup` 一条命令装好；一行安装脚本（`get.sh`）。 |
| 0.4.0 | 2026-11-05 | 从 frame-eac 改名：重点是 PC 版 VRChat，不是泛泛的 EAC 游戏。 |
| 0.3.0 | 2026-11-05 | DXVK 显存上报 1 GB + 4 个编译线程（CPU −15%）、关 tabtip（−135 MB）；`resolution`；基准里有 SteamVR 帧数；三语文档。 |
| 0.1–0.2 | 2026-11-03 | 第一次：在头显上编译 steamac 的补丁版 FEX；EAC `301` / `Session Begin: Success`；VRChat 能在 VR 里玩。 |

## 简要结论

- **有用：** 补丁版 FEX（没有它 EAC 起不来）；上面的默认参数；1920 下的 DFR（着色器复杂的世界帧数 +15–25%，轻量世界在噪声内；省 GPU 时间不省内存）。
  DFR 的 DLL 必须放在 Proton 自己的 DXVK 目录：放 `VRChat.exe` 旁边会在 Wine 加载器里崩。
- **没用：** Vulkan thunk（VR 里 SteamVR 的 x86 客户端提交一帧时崩；只能桌面模式，CPU −10%）；关 FEX TSO（约 35 秒崩）；关 HalfBarrier TSO（没收益）；
  Mesa 的显存上限（只在原生侧生效）；1440 下的 DFR（没收益）；更粗的 DFR（1920 时 `ext` 不比 `def` 快）。
- **瓶颈：** 内存（SteamVR、Steam 界面和追踪占 16 GB 里约 7 GB）和 CPU（FEX 翻译 x86：1056 时每帧约 20 ms，重负载世界 1920 时约 35–40 ms）。
  SteamVR 按 72 Hz 的一半或三分之一给游戏定步。
- **测量：** 读游戏完整的 `/proc/<pid>/smaps` 会卡（采样用 `smaps_rollup`）；无人值守测试前关掉头显休眠（待机会暂停渲染、卡住登录）。

## 未解决

1. 1920 时人多的世界的内存（单人重负载世界还剩约 1.5 GB）。
2. 世界视频播放器的硬件解码（现在是模拟的软件解码）。
3. 重启头显能让 Steam 界面的进程省多少内存。
