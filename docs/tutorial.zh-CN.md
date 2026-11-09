# 安装与使用教程

[English](tutorial.md) · **中文** · [日本語](tutorial.ja.md)

本教程介绍如何在 Steam Frame 上安装 `frame-vrc` 并运行 PC 版 VRChat（含 EasyAntiCheat）。全程耗时约 10 分钟（另需下载约 1.1 GB 的游戏更新）。使用前请先阅读 [README](../README.zh-CN.md) 中的免责声明。

## 版本切换原理

`frame-vrc` 通过配置 Steam 的兼容工具来切换 VRChat 版本：

| 版本 | Steam 兼容工具 |
|---|---|
| **官方版**（安卓构建） | 默认（Lepton） |
| **PCVR 版**（Windows 构建 + DFR + H.264 硬件解码） | Proton Experimental (Ji10me PCVR Edit) |

切换版本后，下次启动 VRChat 时 Steam 会自动下载并更新为对应平台的构建。补丁版 FEX 仅对 VRChat 生效，不影响系统自带的 FEX-Emu 及其他游戏。

## 1. 安装 frame-vrc

在头显的 Konsole 终端（或通过 SSH）中，以普通用户 `steamos` 执行以下命令（**请勿使用 `sudo`**）：

```sh
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash
```

安装耗时约 1 分钟。完成后打开 VR 面板：在 SteamVR 仪表板上点 **+**（启动程序）> **frame-vrc**。

![SteamVR 仪表板：+（启动程序）> frame-vrc](images/steamvr-launch-program.jpg)

## 2. 配置 PCVR 版

进入面板的 **Install** 页面：

![Install 页：Setup to PCVR Version](images/panel-install-setup.jpg)

1. 点击 **Setup to PCVR Version**（绿色按钮），再次点击确认。
2. 首次配置会**自动重启一次 Steam** 以加载新增的兼容工具（界面会短暂消失约 1 分钟，随后自动继续配置）。可在 **Log** 页面查看实时进度。
3. 若尚未安装 VRChat，过程中会弹出 Steam 安装提示，确认即可。

![设置过程中的 Log 页](images/panel-log.jpg)

配置完成后，面板状态将显示为 **patched: update and launch VRChat**。

## 3. 启动 VRChat

从面板 **Home** 页或 Steam 库中启动 VRChat。Steam 会先下载 PC 版本更新（约 1.1 GB），随后启动游戏。首次进入世界需要编译着色器，耗时数分钟，后续进入会明显加快。

![Steam 正在把 VRChat 更新为 PC 版](images/steam-vrchat-updating.jpg)

配置完成后，VRChat 的“属性 > 兼容性”设置如下（启动参数会自动设为 `frame-vrc-run %command%`，请勿手动修改）：

![兼容工具：Proton Experimental (Ji10me PCVR Edit)](images/compat-pcvr-version.jpg)

## 4. 面板功能说明

面板左下角可切换界面语言（English / 日本語）。当 VRChat 未安装或处于官方版时，仅显示 **Home** 与 **Install** 页面；切换至 PCVR 版后将显示完整页面：

- **Home**：查看内存与 CPU 占用、调整单眼渲染分辨率（`1056` / `1440` / `1920`，默认 `1920`）以及启动 VRChat。注视点渲染（DFR）与 H.264 硬件解码默认常开。

  ![Home 页](images/panel-home.jpg)

- **Environment**：展示各项运行环境检查结果，异常项会提供一键修复按钮。

  ![Environment 页](images/panel-environment.jpg)

- **Advanced**：查看已安装组件版本、更多分辨率档位、Swap 状态，以及执行更新或卸载。

  ![Advanced 页](images/panel-advanced.jpg)

## 5. 切回官方安卓版

在 **Install** 页面点击 **Recover to Official Android Version**（红色按钮）并再次点击确认。兼容工具将恢复为默认的 Lepton，并还原原有的启动参数与渲染分辨率。下次启动 VRChat 时，Steam 会自动下载回安卓版。

![Install 页：Recover to Official Android Version](images/panel-install-recover.jpg)

![兼容工具回到默认（Lepton）](images/compat-official-lepton.jpg)

## 命令行工具

面板的所有操作均可通过终端命令完成：

```text
frame-vrc setup | official | status | doctor
frame-vrc resolution 1056|1440|1920
frame-vrc launch | update | uninstall [--purge]
```

更多说明详见 [安装与命令说明](install.zh-CN.md) 与 [文件变更与卸载还原](changes.zh-CN.md)。
