# 安装

[English](install.md) · **中文** · [日本語](install.ja.md)

所有操作都在 Frame 上（Konsole 或 SSH），以普通用户（`steamos`）运行。不要用 sudo。

## 1. 前提条件

1. 在 Steam 里安装 **Proton Experimental** 和 **Proton EasyAntiCheat Runtime**（没有 **FEX-Emu** 的话也装上：库 > 工具）。
   装好 frame-vrc 后，`frame-vrc install-deps` 会为每个缺的组件打开 Steam 的安装确认。
2. **VRChat > 属性 > 兼容性 > Proton Experimental。** 不设的话 Steam 会装 Android 版。让 Steam 下载 Windows 版。
3. 先启动一次 SteamVR（frame-vrc 通过它设渲染分辨率）。第一次设置时头显插着电。

## 2. 安装 frame-vrc

```sh
# a) 从 GitHub：安装最新 release（自己校验 SHA256SUMS）
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash

# b) 用下载好的 frame-vrc-<版本>.tar.gz：先校验再安装
sha256sum -c --ignore-missing SHA256SUMS        # 在放 tarball 和 SHA256SUMS 的文件夹里
tar xzf frame-vrc-<版本>.tar.gz && sh frame-vrc-<版本>/install.sh
```

装到 `~/.local/lib/frame-vrc`，链接为 `~/.local/bin/frame-vrc`（找不到 `frame-vrc` 就开个新 shell）。

## 3. 设置

```sh
frame-vrc setup             # 推荐：补丁版 FEX、DFR 版 Proton、1920 + DFR、测过的参数
frame-vrc setup --no-dfr    # 不要 DFR：原版 Proton Experimental
frame-vrc launch            # 或者在 Steam 里启动 VRChat（VR 模式）
```

Steam 里缺东西时 `setup` 会停下并说明要做什么；做完再运行一次。它从最新 release 下载两个包，每个都用 `.sha256` 校验：

| 包 | 内容 | 不下载的话 |
|---|---|---|
| `fex-eac-<版本>-steamframe-aarch64.tar.gz` | 补丁版 FEX | `frame-vrc setup 文件` 或 `frame-vrc fetch 文件`；或在头显上编译：`frame-vrc build`（约 5 分钟，会很热，到 100 °C 自动停） |
| `dfr-dxvk-<提交>-steamframe.tar.gz` | DFR 版 DXVK（`d3d11.dll`、`dxgi.dll`）、眼动 bridge 和它的 OpenVR 库 | `frame-vrc setup --dfr 文件` 或 `frame-vrc proton dfr 文件`；或自己编译（见下） |

从别处拿的包，把它的 `.sha256` 放在旁边。

第一次 `proton dfr` 会把 Proton Experimental 复制到 `~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr`（约 1.5 GB），换掉其中的 DXVK，
并重启一次 Steam 让它认出这份副本。Steam 更新 Proton Experimental 后 `doctor` 会提示，再运行一次 `frame-vrc proton dfr` 就会重新复制。

### 自己编译 DFR 包

按 [ji10me/dxvk-steam-frame-dfr](https://github.com/ji10me/dxvk-steam-frame-dfr) 的说明编译（DXVK 用 MinGW-w64：
`meson setup build-frame --cross-file build-win64.txt --buildtype release -Denable_d3d8=false -Denable_d3d9=false -Denable_d3d10=false`；
bridge 在 Linux ARM64 上对 OpenVR SDK `0924064` 编译，链接时加 `-Wl,-rpath,'$ORIGIN'`），再用本仓库的
`tools/make-dfr-package.sh DXVK源码 OPENVR_SDK` 打包。

## 4. DFR 怎么运行

用 DFR 版 Proton 时，VRChat 的启动选项会以 `…/lib/dfr-run` 开头。它在游戏之前原生运行：按选的级别（`frame-vrc dfr`；`auto` = 分辨率 1600 起自动开）
设好 DXVK 的 DFR 变量，并启动眼动 bridge；bridge 等 VRChat 拿到 VR 场景后开始工作，随游戏退出（日志：`~/.local/share/frame-vrc/bridge.log`）。
没戴头显时注视点数据无效，DXVK 按全速着色。

## 5. 手动（不用 frame-vrc）

1. 解开 FEX 包（得到 `~/fex-eac-work/stage`），退出所有游戏（`pgrep -x FEXServer` 没有输出），运行 `sh lib/install-fex-tool.sh`
   （`restore` 换回 Valve 的文件）。
2. 粘贴启动选项（在装了工具的机器上用 `frame-vrc launch-options show` 打印），除非按 ji10me 的说明配好了 DFR，否则去掉 `…/dfr-run`。
3. 分辨率：`/opt/steamvr/bin/linuxarm64/vrcmd --set-settings-int steam.app.438100.resolutionOverride 1920`
   （先用 `--settings-int steam.app.438100.resolutionOverride` 记下原值）。

## 6. 省内存的 VRChat 设置

在 VRChat 里（设置，在 VR 中）：隐藏 Poor / Very Poor 模型，调低模型下载和解压大小上限，调低最多显示的模型数，打开粒子限制，镜子调低或关掉。
玩之前重启头显能让 Steam 界面的进程重新开始。

## 7. 确认能用

`frame-vrc doctor` 最后应该是 `all set`。要记录一次：先运行 `frame-vrc bench` 再启动游戏；`~/.local/share/frame-vrc/bench/` 里的报告会写明 EAC 是否启动
（`Launcher finished with: 301`、`AntiCheat Session Begin: Success`）。游戏起不来的话，在启动选项里临时加一次 `PROTON_LOG=1`，看 `~/steam-438100.log`。

全部恢复：[changes.zh-CN.md](changes.zh-CN.md)。
