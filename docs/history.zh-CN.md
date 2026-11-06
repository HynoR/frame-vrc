# 历史

[English](history.md) · **中文** · [日本語](history.ja.md)

一台 Steam Frame（SteamOS 0.4.5 vr，骁龙 8 Gen 3，16 GB），VRChat Windows 版跑在 Proton Experimental 上。
实测数据：[research/summary.zh-CN.md](research/summary.zh-CN.md)。

## 版本

| 版本 | 日期 | 改动 |
|---|---|---|
| 0.7.1 | 2026-11-06 | 修复：从 VR 面板写入的启动选项把游戏的 CPU 只映射到 0-4 号核心（SteamOS 把通过 `systemd-run` 启动的进程限制在这几个核心），VRChat 用不到 5-7 号大核，帧数变低。现在 CPU 列表始终覆盖全部 8 个核心。`cpus` 只接受 8、16、32（8 的倍数才能均匀映射到所有核心；少于 8 会漏掉大核）。重测 8 和 16（1920 + DFR、重负载世界、5 轮有效）：帧数（16-20）和内存都没有明显差别，默认保持 16。 |
| 0.7.0 | 2026-11-06 | DFR `auto` 改为 1440 起开启（原来 1600）。VR 面板：SteamVR 应用列表里的 **frame-vrc**（安装时在头显上编译）：当前设置、环境检查（每个问题一个修复按钮：在 Steam 里安装、FEX、Proton、启动选项）、推荐配置、分辨率、DFR、启动游戏、高级开关、监控页（内存、swap、每核 CPU、温度、VRChat 的内存和 CPU），日志页（setup 的每一步），英文或日文界面；设置类按钮合并后一次应用（`frame-vrc apply`）。`setup` 输出带编号的步骤，SteamVR 重启前提示并等 10 秒；`launch` 通过 Steam 客户端 API 启动并确认游戏已启动；`fex eac` 会自己下载包。新增 `status --json`、`doctor --json`、`update`、`panel`；`install-deps` 也会安装 VRChat（先设成 Proton Experimental）。 |
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
  Mesa 的显存上限（只在原生侧生效）；1440 下的 DFR 基准测试（帧数在噪声内，但实际游玩有帮助，所以 0.7.0 起 `auto` 从 1440 开启）；更粗的 DFR（1920 时 `ext` 不比 `def` 快）。
- **瓶颈：** 内存（SteamVR、Steam 界面和追踪占 16 GB 里约 7 GB）和 CPU（FEX 翻译 x86：1056 时每帧约 20 ms，重负载世界 1920 时约 35–40 ms）。
  SteamVR 按 72 Hz 的一半或三分之一给游戏定步。
- **测量：** 读游戏完整的 `/proc/<pid>/smaps` 会卡（采样用 `smaps_rollup`）；无人值守测试前关掉头显休眠（待机会暂停渲染、卡住登录）。

## 未解决

1. 1920 时人多的世界的内存（单人重负载世界还剩约 1.5 GB）。
2. 世界视频播放器的硬件解码（现在是模拟的软件解码）。
3. 重启头显能让 Steam 界面的进程省多少内存。
