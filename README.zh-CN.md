# frame-vrc

[English](README.md) · **中文** · [日本語](README.ja.md)

> **只适合愿意折腾的技术玩家。** 这是非官方的研究项目，面向会用 Linux 命令行、会看日志、出了问题能自己回退的人。
> 如果你不是，请等官方支持，新手不要轻易尝试。风险自负，见[免责声明](#免责声明)。

在 **Steam Frame** 上单机运行 **PC（Windows）版 VRChat**（包括 Easy Anti-Cheat），用的是 [StonkGuy/FEX](https://github.com/StonkGuy/FEX)
的补丁版 FEX（`eac-*` 标签，来自 [steamac](https://github.com/StonkGuy/steamac) 的 `fex-eac/`）。一个工具负责检查环境、
带备份地切换 FEX 版本、设置启动选项和渲染分辨率、记录可对比的基准数据。

不绕过 EAC：运行的是 Valve 的 Proton EasyAntiCheat Runtime 里原版的 EAC 客户端，补丁版 FEX 只是让模拟足够精确。

目标是**玩 VRChat**。其他带 EAC 的游戏不是目标，也没有测试过：补丁版 FEX 不一定能跑，这里的设置也是按 VRChat 调的。

**研究阶段**，验证过一台头显（SteamOS 0.4.5 vr，2026 年 11 月）。

## 现状

| FEX 版本（`frame-vrc switch …`） | VR | 桌面模式（`--no-vr`） |
|---|---|---|
| `valve`：Steam 自带的 FEX-Emu | EAC 失败（这就是要打补丁的原因；在 steamac 的 Mac 环境里看到的） | 同左 |
| `eac`：steamac 补丁版 FEX | **能玩**：EAC 正常，能进世界；1440 约 20 fps，720 约 36 fps | 能玩 |
| `thunk`：`eac` + Vulkan thunk | 第一帧交给 SteamVR 时崩溃（留给 FEX/Valve） | 能玩，CPU 少约 10% |

瓶颈是内存：SteamVR、Steam 界面和追踪服务占掉 16 GB 里的约 7 GB，人多的世界里 VRChat 会把剩下的和 zram 都用满。
见[研究发现](docs/history.zh-CN.md#发现)。

## 快速开始（用打包好的 FEX）

在 Frame 上（Konsole 或 SSH），用普通用户（不要 sudo）：

```sh
curl -fsSL https://github.com/HynoR/frame-vrc/releases/latest/download/get.sh | bash   # 安装 frame-vrc
frame-vrc install-deps                     # 让 Steam 安装 FEX-Emu、Proton Experimental、EAC Runtime（在 Steam 里确认）
# Steam：VRChat > 属性 > 兼容性 > Proton Experimental，然后让它下载 Windows 版
frame-vrc fetch https://github.com/HynoR/frame-vrc/releases/download/<tag>/fex-<version>-steamframe-aarch64.tar.gz
frame-vrc switch eac && frame-vrc launch-options set
frame-vrc resolution 720                   # 可选：想清楚一点用 1056，`reset` 恢复原值
frame-vrc doctor                           # 全部 "ok" 后在 VR 里启动 VRChat（第一次进世界约 3 分钟）
```

其他方式（Discord 上下载的包、在头显上自己编译、完全手动）：[docs/install.zh-CN.md](docs/install.zh-CN.md)。

## 文档

| | |
|---|---|
| [安装](docs/install.zh-CN.md) | 前提条件；用打包好的、自己编译或完全手动安装；VRChat 设置；检查是否正常 |
| [改了什么、怎么恢复](docs/changes.zh-CN.md) | 动过的每个文件和设置、在哪、怎么恢复、完整卸载 |
| [实验历史和发现](docs/history.zh-CN.md) | 按时间顺序：试了什么、测了什么、学到什么；待解决的问题 |
| [编译 Vulkan thunk](docs/thunk-build.zh-CN.md) | 可选的 `thunk` 版本 |
| [实验记录](docs/research/) | 原始记录 |

## 命令

```text
frame-vrc doctor [game]                    检查所有项目，告诉你要修什么
frame-vrc install-deps [game]              让 Steam 安装 FEX-Emu、Proton Experimental、EAC Runtime
frame-vrc build [--jobs N]                 在头显上编译补丁版 FEX（约 5 分钟，会很热，100°C 自动停）
frame-vrc fetch URL|FILE                   用打包好的版本（.sha256 放旁边）：校验、放进 stage
frame-vrc pack [eac|thunk]                 把你的版本打包给别人用
frame-vrc switch status|valve|eac|thunk    切换 Steam 的 FEX-Emu（先备份 Valve 的文件）
frame-vrc launch-options show|set|clear    VRChat 的启动选项，通过 Steam 自己的 API
frame-vrc resolution show|N|reset          VRChat 在 SteamVR 里的渲染分辨率，可以低于界面最低的 1440
frame-vrc bench [game]                     记录一局的 CPU、内存、温度、SteamVR 帧数和 EAC 结果
frame-vrc uninstall [--purge]              全部撤销
```

## 安全

用普通用户运行，从不用 sudo，只写 `~` 下面；不改系统、固件和 `/persist`。每个改动都能撤销（[docs/changes.zh-CN.md](docs/changes.zh-CN.md)）。
只影响 Steam 用 FEX-Emu 兼容工具（x86 Proton）运行的游戏。

## 参与研究

把 `frame-vrc bench` 的报告（`~/.local/share/frame-vrc/bench/`）用 **Benchmark report** issue 模板或在 Discord 上分享：
同一个世界、同样时长、一次只改一项，写明改了什么。不会自动上传任何东西。

## 免责声明

- **不适合新手。** 画面会糊、帧数低、第一次加载很久、可能崩溃，需要看日志。出了问题你得能照着 [docs/changes.zh-CN.md](docs/changes.zh-CN.md) 自己恢复。
- **非官方。** 与 Valve、VRChat、Epic Games（Easy Anti-Cheat）、FEX-Emu、FX GAMES 无关，也没有得到它们的认可。它们都不支持在 Frame 上运行 PC 版，
  Steam、SteamVR、Proton 或 VRChat 的任何一次更新都可能让它失效。
- **你的账号。** 没有绕过或修改 EAC，但 VRChat 和 EAC 是否接受这种运行方式由它们决定。不要修改游戏文件。遵守 VRChat 服务条款是你自己的责任。
- **你的硬件。** 编译和游戏时头显会很热（实测最高约 98°C）。插着电、保持通风，觉得不舒服就停。
- **不提供任何担保。** 按 Apache-2.0 许可"按原样"提供，不提供任何形式的担保；作者不对任何损坏、数据丢失或账号处理负责。

## 许可

Apache-2.0，见 [LICENSE](LICENSE) 和 [NOTICE](NOTICE)。
