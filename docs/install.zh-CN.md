# 安装与命令说明

[English](install.md) · **中文** · [日本語](install.ja.md)

图文步骤请参阅 [安装与使用教程](tutorial.zh-CN.md)。所有命令均在 Steam Frame 上（Konsole 或 SSH）以普通用户 `steamos` 运行，**切勿使用 `sudo`**。

## 安装 frame-vrc

### 在线安装

```sh
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash
```

`get.sh` 会自动下载最新发布包与 `SHA256SUMS`，完成校验后执行 `install.sh`。

### 离线包安装

在包含发布包与 `SHA256SUMS` 的目录下执行：

```sh
sha256sum -c --ignore-missing SHA256SUMS
tar xzf frame-vrc-<version>.tar.gz && frame-vrc-<version>/install.sh
```

程序安装路径为 `~/.local/lib/frame-vrc`，可执行文件软链接至 `~/.local/bin/frame-vrc`（若提示找不到命令，请重新打开终端），同时会在本地编译 VR 面板（约耗时 1 分钟）。

## `setup` 执行流程

执行 `frame-vrc setup`（或在面板中点击 **Setup to PCVR Version**）会依次完成以下操作：

1. **检查 Steam 依赖**：检查 FEX-Emu、Proton Experimental 与 Proton EasyAntiCheat Runtime，缺失时自动调起 Steam 安装提示。
2. **下载与校验组件**：从 GitHub Releases 下载补丁版 FEX、DFR DXVK、H.264 视频插件及 Wine 音频修复包，并逐一校验 SHA-256。
3. **部署私有 FEX 环境**：在 `~/.local/share/frame-vrc/fex` 下配置仅对 VRChat 生效的 ji10me 运行方案，并编译会话管理器与眼动追踪桥接程序。
4. **创建独立 Proton 工具**：在 `~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr`（约 1.5 GB）创建集成 DFR DXVK 的 `Proton Experimental (Ji10me PCVR Edit)`；首次创建会自动重启一次 Steam 以刷新工具列表。
5. **配置 VRChat 启动项**：将 VRChat 的兼容工具设为该 Proton 副本，默认单眼渲染分辨率设为 `1920`，启动参数设为 `frame-vrc-run %command%`。

## 命令参考

```text
frame-vrc setup                  配置并切换至 PCVR 版
frame-vrc official               恢复至官方安卓版
frame-vrc status                 查看当前版本与组件状态
frame-vrc doctor                 检查运行环境并给出修复建议（正常时输出 all set）
frame-vrc resolution 1056|1440|1920  设置单眼渲染分辨率
frame-vrc launch                 启动 VRChat
frame-vrc update                 更新 frame-vrc 及运行组件
frame-vrc uninstall [--purge]    回退至官方版并卸载 frame-vrc
frame-vrc dfrtest [on|off]       开启/关闭 DFR 调试（画面显示注视点并记录 DXVK DFR 日志）
frame-vrc video check            播放视频时检测 H.264 硬件解码是否生效
```

## VRChat 内存优化建议

为避免 16 GB 内存耗尽，建议在 VRChat 内调整以下设置：
- **模型安全与显示限制**：屏蔽 Poor / Very Poor 级别模型，调低模型最大下载体积、解压后体积上限及同屏最大显示人数。
- **画面设置**：开启粒子限制（Particle Limiter），降低或关闭镜子画质，关闭抗锯齿（Anti-Aliasing），其余画质选项设为 Low。

## 故障排查

若游戏无法启动：
1. 运行 `frame-vrc doctor` 检查环境状态。
2. 如需采集详细日志，可在 Steam 的 VRChat 启动参数 `%command%` 前临时添加 `PROTON_LOG=1`（即 `PROTON_LOG=1 frame-vrc-run %command%`），复现后查看 `~/steam-438100.log`（排查完毕后请移除 `PROTON_LOG=1`）。
3. 提交问题反馈时，请附上 `frame-vrc status` 与 `frame-vrc doctor` 的完整输出。

关于文件变更明细与完整卸载方法，请参阅 [文件变更与卸载还原](changes.zh-CN.md)。
