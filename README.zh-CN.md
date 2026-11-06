# frame-vrc

[English](README.md) · **中文** · [日本語](README.ja.md)

> **只适合爱折腾的人。** 这是非官方的研究项目，面向会用 Linux 命令行、会看日志、出了问题能自己恢复的人。
> 如果不是，请等官方支持。风险自负，见[免责声明](#免责声明)。

在 **Steam Frame** 上独立运行 **PC（Windows）版 VRChat**（包括 Easy Anti-Cheat），用的是
[StonkGuy/FEX](https://github.com/StonkGuy/FEX) 的补丁版 FEX（`eac-*` 标签，来自 [steamac](https://github.com/StonkGuy/steamac) 的 `fex-eac/`），
可选注视点渲染（DFR，来自 [ji10me/dxvk-steam-frame-dfr](https://github.com/ji10me/dxvk-steam-frame-dfr)）。
一个工具负责在官方版和补丁版之间切换、设置测过的参数、启动游戏、全部还原。

EAC 没有被绕过：跑的是 Valve 的 Proton EasyAntiCheat Runtime 里原版的 EAC 客户端，补丁版 FEX 只是让模拟足够准确。
目标是**玩 VRChat**，其他带 EAC 的游戏没有测过。

**研究阶段**，验证过一台头显（SteamOS 0.4.5 vr，2026 年 11 月）。

## 快速开始

1. 在 Steam 里：安装 **Proton Experimental** 和 **Proton EasyAntiCheat Runtime**；**VRChat > 属性 > 兼容性 > Proton Experimental**，
   让它下载 Windows 版。没有 **FEX-Emu** 的话也装上（库 > 工具）。
2. 在 Frame 上（Konsole 或 SSH，普通用户，不要 sudo），SteamVR 开着：

```sh
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash   # 安装 frame-vrc
~/.local/bin/frame-vrc setup
~/.local/bin/frame-vrc launch
```

`setup` 按推荐配置设好：补丁版 FEX、DFR 版 Proton、渲染分辨率 1920 + DFR，以及下面那些测过的参数。两个包都从最新 release 下载并校验 SHA-256。
Steam 会重启一次（让它认出新的 Proton）。第一次进世界约 3 分钟，之后约 20 秒。

还原成官方版：`frame-vrc uninstall`。细节和其他安装方式：[docs/install.zh-CN.md](docs/install.zh-CN.md)。

## 开关

| 命令 | 选项 | 默认 |
|---|---|---|
| `frame-vrc fex` | `valve` Steam 的 FEX · `eac` 补丁版（EAC 需要它） | eac |
| `frame-vrc proton` | `stock` Proton Experimental · `dfr` 它的副本，换上 DFR 版 DXVK（Steam 自己那份不动） | dfr |
| `frame-vrc resolution` | `1056` 性能 · `1440` 均衡 · `1920` 画质；高级：`1600`、`2160`、任意 N | 1920 |
| `frame-vrc dfr` | `auto` 1600 及以上自动开 · `off` · `def`；高级：`wide`、`ext`（适合 2160 及以上） | auto |
| `frame-vrc tune`（高级） | `cpus N` · `dxvk-threads N` · `vram MB` · `reset` | 16 · 4 · 1024 |

中间的分辨率用 SteamVR 自己的设置就能调；列出 1056 是因为 SteamVR 界面最低只到 1440。
`frame-vrc status` 显示当前全部设置；`frame-vrc doctor` 检查一切并说明怎么修。

固定开启（VR 里实测过、没有值得做成开关的副作用）：FEX 代码缓存只给 VRChat（Wine 辅助进程 −640 MB）、FEX 翻译块 500 条指令
（游戏内存 −0.65 GB、zram −1.9 GB）、关掉 Wine 的触摸键盘（−135 MB）、桌面镜像窗口 640×480。

## 有用的和没用的

- **有用：** 补丁版 FEX（没有它 EAC 起不来）、上面的参数、**1920 下的 DFR**：重负载世界帧数 +15–25%（着色器越复杂收益越大，轻量世界在噪声内）。
  DFR 省的是 GPU 时间，不省内存。
- **没用：** Vulkan thunk（VR 里崩）、关 FEX TSO（崩）、1440 下的 DFR、更粗的 DFR（1920 时 `ext` 不比 `def` 快）、低于 1440 的分辨率（只作应急）。

瓶颈是内存：SteamVR、Steam 界面和追踪服务占掉 16 GB 里的约 7 GB。单人在重负载世界、1920 时还剩约 1.5 GB。
数据：[docs/research/summary.zh-CN.md](docs/research/summary.zh-CN.md)。

## 命令

```text
frame-vrc setup [--no-dfr] [--resolution N] [--dfr URL|FILE] [FEX 的 URL|FILE]
frame-vrc status | doctor | launch
frame-vrc fex valve|eac             frame-vrc proton stock|dfr [URL|FILE]
frame-vrc resolution 1056|1440|1920 frame-vrc dfr auto|off|def
frame-vrc uninstall [--purge]       还原 Valve 的 FEX、Proton Experimental、你原来的启动选项和分辨率
高级：resolution N · dfr wide|ext · tune · install-deps · build · fetch · pack · launch-options · bench
```

## 文档

| | |
|---|---|
| [安装](docs/install.zh-CN.md) | 前提条件；用包、自己编译、手动安装；确认能用 |
| [改了什么、怎么恢复](docs/changes.zh-CN.md) | 动过的每个文件和设置、怎么恢复、完整卸载 |
| [历史](docs/history.zh-CN.md) | 按版本：试过什么、学到什么 |
| [研究总结](docs/research/summary.zh-CN.md) | 实测数据 |

## 安全

以普通用户运行，从不 sudo，只写 `~`；系统、固件、`/persist` 都不动。每个改动都能恢复（[docs/changes.zh-CN.md](docs/changes.zh-CN.md)）。
只改 VRChat 的设置和 Steam 的 FEX-Emu 工具；Proton Experimental 本身从不修改（DFR 用的是副本）。

## 帮忙做研究

把 `frame-vrc bench` 的报告（`~/.local/share/frame-vrc/bench/`）用 **Benchmark report** issue 模板或在 Discord 分享：同一个世界、同样时长、
一次只改一样，并说明改了什么。不会自动上传任何东西。

## 致谢和相关项目

- [steamac](https://github.com/StonkGuy/steamac) 和 [StonkGuy/FEX](https://github.com/StonkGuy/FEX)（FX GAMES）：让 EAC 在 Proton 下能用的补丁版 FEX，
  以及这个工具起步时用的 `fex-eac/` 编译和安装脚本。
- [ji10me/dxvk-steam-frame-dfr](https://github.com/ji10me/dxvk-steam-frame-dfr)（[@ji10me](https://x.com/ji10me)），基于 ReinaS-64892 和 poprox24 的
  DFR 工作：注视点渲染 DXVK 和眼动 bridge。
- [FEX-Emu](https://github.com/FEX-Emu/FEX)：这里一切都跑在它上面的 x86 模拟器。
- Valve：Proton、Proton EasyAntiCheat Runtime、SteamVR、OpenVR 和 Steam Frame 的 FEX-Emu 工具。
- [DXVK](https://github.com/doitsujin/dxvk) 和 Mesa 的 Turnip 驱动：Frame GPU 上的 Direct3D 和 Vulkan。
- [Decky Loader](https://github.com/SteamDeckHomebrew/decky-loader)：设置启动选项用的 Steam CEF 方法。

## 免责声明

- **不适合新手。** 画面模糊、帧数低、第一次加载很久、会崩、要看日志。出了问题你要能按 [docs/changes.zh-CN.md](docs/changes.zh-CN.md) 自己恢复。
- **非官方。** 与 Valve、VRChat、Epic Games（Easy Anti-Cheat）、FEX-Emu、FX GAMES 都没有关系，也没有得到它们认可。它们都不支持在 Frame 上跑 PC 版，
  Steam、SteamVR、Proton 或 VRChat 的任何一次更新都可能让它失效。
- **你的账号。** EAC 没有被绕过或修改，但 VRChat 和 EAC 是否接受这种环境由它们决定。不要修改游戏文件。遵守 VRChat 服务条款是你自己的责任。
- **你的硬件。** 编译和游戏会让头显很热（实测最高约 98 °C）。插着电、保持通风，不舒服就停。
- **不提供担保。** 按 Apache-2.0 许可"按原样"提供，不提供任何担保；作者不对任何损坏、数据丢失或账号处置负责。

## 许可

Apache-2.0，见 [LICENSE](LICENSE) 和 [NOTICE](NOTICE)。DFR 包带有自己的许可（DXVK 和 Frame 部分：zlib；GAZ2 头文件：MIT；OpenVR：BSD-3-Clause），
在 `dfr/licenses/` 里。
