# 实验历史和发现

[English](history.md) · **中文** · [日本語](history.ja.md)

一台 Steam Frame（SteamOS 0.4.5 vr、骁龙 8 Gen 3、16 GB），VRChat Windows 版，Proton Experimental。原始记录：[research/](research/)。

## 时间线

### 2026-11-03：带 EAC 的 VRChat 在 VR 里跑起来了

- 在头显上编译 steamac 的 `fex-eac`（让 EAC 能在 Proton 下运行的补丁版 FEX，已在 Mac 上验证过）：4 个线程约 5 分钟、约 2 GB、最高约 98°C。
  带 Valve 文件的备份换进 Steam 的 FEX-Emu；每一步都能撤销。
- 启动选项通过 Steam 自己的 API（CEF 调试端口）设置，不用在 VR 里打字。
- 结果：EAC `Launcher finished with: 301`、`AntiCheat Session Begin: Success`，**在 VR 里**进了世界。第一次进世界约 3 分钟（FEX 代码缓存是冷的），之后约 20 秒。
- 基线（VR，1440）：游戏 CPU 约 280%（100% = 一个核），游戏内存（PSS）平均 4.4 GB / 峰值 5.4 GB，GPU 显存约 2.2 GB，可用内存最低约 0.7 GB，zram 用满，约 94°C。
- Vulkan thunk（原生 Vulkan 代替模拟）：Valve 预编译的和用 eac 版 FEX 编的都让游戏卡在加载。
- `WINE_CPU_TOPOLOGY` 16 对 8：看不出明显差别（世界不同）。
- frame-vrc 0.1–0.2：在 Valve / eac / thunk 版本之间切换、启动选项、基准采样。

### 2026-11-05：thunk 诊断、桌面模式、VR 内存和帧数

- **thunk 原因 1，已修正：** 原生 Vulkan loader 拿到的是容器给 x86 的驱动清单（路径只有模拟侧看得到），找不到驱动。
  `FEX_HOSTENV=VK_DRIVER_FILES=<json>` 让它用宿主机的 Turnip。
- **thunk 原因 2，留给 FEX/Valve：** VR 里 SteamVR 的 x86 vrclient 通过 thunk 提交第一帧时游戏崩溃。要改 FEX 的 thunk ABI 代码才能修。
- **桌面模式**开不开 thunk 都能玩，画面正常；thunk 省约 10% CPU，不省内存。
- **VR 内存和帧数**（home 世界单人，每轮 5 分钟，自动化）：

  | 轮 | 改动 | fps | 游戏 GPU 显存 | zram 峰值 | 可用内存最低 |
  |---|---|---|---|---|---|
  | A–C | 基线（显存上限实验，结果其实都没生效） | 约 20 | 约 2.4 GB | 约 7.5 GB | 约 0.8 GB |
  | D | DXVK 报 1 GB 显存 + 4 个编译线程，关掉 tabtip | 约 20，CPU −15% | 2.4 GB | 7.4 GB | 1.1 GB |
  | E | D + 分辨率 1056 | 29 | 2.1 GB | 7.2 GB | 0.9 GB |
  | F | D + 分辨率 720 | 36（SteamVR 统计） | 1.8 GB | 6.5 GB | 1.1 GB |

- frame-vrc 0.5.2：默认启动选项加入 `FEX_MAXINST=500`；推荐分辨率改为 1056。
- frame-vrc 0.5.3：FEX 代码缓存只给 VRChat 和 EAC 启动器（`FEX_APP_CONFIG`）：Wine 辅助进程少约 640 MB，帧数不变（VR、1056 下 35.3 对 33.9）。
- frame-vrc 0.5.2（另外）：全新安装（还没有 Valve 备份）时 `setup` 和 `switch` 不再无声退出；没有 Steam 配置或 SteamVR 时 `doctor` 和 `resolution` 也不再无声退出。由干净安装测试发现。
- 安装脚本（`get.sh`）改为从仓库直接获取，不再放在 release 里，修它不用重新发布；装的仍然只是已发布的版本。
- frame-vrc 0.5.1：一行安装命令（`get.sh`）安装成功后不再报错退出。
- frame-vrc 0.5.0：`setup` 一条命令完成整个安装；`switch` 在启动选项为空时也会自动填上。
- 从 frame-eac 改名为 **frame-vrc**（0.4.0）：重点是玩 PC 版 VRChat，而不是泛泛地运行 EAC 游戏。
- frame-vrc 0.3.0：D 的设置成为默认、`resolution` 命令、基准报告带 SteamVR 帧数、thunk 版本打包、`fetch` 支持本地文件、三语文档。

## 发现

- **渲染分辨率是最大的杠杆。** SteamVR 设置界面最低 1440；`vrcmd`（和 `frame-vrc resolution`）可以更低；SteamVR 按 32 对齐（1080 → 1056）。
- **CPU 和 GPU 都接近极限。** 1056 时每帧 CPU 约 20 ms、GPU 约 15 ms；SteamVR 按 36 或 24 fps 给游戏定步（36 需要 27.7 ms 内）。CPU 时间大多花在 FEX 翻译 x86 代码上。
- **游戏看到的显存：** Steam 的 Mesa 显存上限只在原生侧生效；`eac` 下 Unity 看到真实的 14683 MB。DXVK 的 `dxgi.maxDeviceMemory` 有效。
- **FEX 选项（只影响 VRChat，用启动选项里的 `FEX_*`）：** Steam 没给 VRChat 任何 FEX 调优（全是 FEX 默认值）。关 TSO 会让 VRChat 约 35 秒内崩溃；
  关 HalfBarrier TSO 没有收益；`MaxInst=500`（Valve 在 Proton ARM64 的值）游戏内存约 −0.65 GB、zram 约 −1.9 GB，帧数约 −10%（两种顺序都一样），现在是默认。
- **DXVK 编译线程**跟着报告的 CPU 数（16）；改成 4 CPU 少约 15%。
- **注视点渲染：** SteamOS 的 `VK_LAYER_VALVE_fdm_injection` 只有 aarch64 版本，所以模拟运行的游戏用不上。开 thunk 时会加载，这就是 VR thunk 崩溃值得修的原因。
- **内存都去哪了：** 只开 SteamVR 时 16 GB 里可用约 8.7 GB。Steam 界面（约 1.5 GB + zram 里 1.4 GB）、SteamVR 仪表盘（约 0.7 GB）、XRService（约 0.7 GB）
  在 VR 里都要用。游戏里每个 Wine 辅助进程约占 135 MB，其中一半左右是 FEX 分配器。
  那部分是每个进程各自载入的 FEX 代码缓存索引（每套 FEX 设置一份，最大 124 MB）；只给游戏开缓存后，14 个辅助进程从 973 MB 降到 333 MB。
- **测量注意：** 读游戏完整的 `/proc/<pid>/smaps` 会让它卡顿；frame-vrc 的采样用 `smaps_rollup`。`vrcmd --stats` 能拿到游戏的帧数和每帧 CPU/GPU 时间。

## 待解决

1. 人多的世界里 1 GB 显存报告和 VRChat 模型限制的效果。
2. 重启头显能让 Steam 界面的进程少占多少。
3. VR thunk 崩溃（上游）。用 OpenVR 转 OpenXR 的层（比如 xrizer）替换 SteamVR 的 x86 vrclient 也许能绕开；没测过，而且因为 EAC 不能改游戏文件。
