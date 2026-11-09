# 文件变更与卸载还原

[English](changes.md) · **中文** · [日本語](changes.ja.md)

`frame-vrc` 仅在当前用户的家目录（`~`）下读写文件，全程不使用 `sudo`。以下为涉及的全部配置与路径。

## Steam 与 SteamVR 配置变更（仅限 VRChat）

| 配置项 | 修改命令 | 还原方式 |
|---|---|---|
| VRChat 兼容工具：`Proton Experimental (Ji10me PCVR Edit)` | `setup` | 执行 `official`（恢复为默认 `Lepton`） |
| VRChat 启动参数：`frame-vrc-run %command%` | `setup` | 执行 `official`（从备份 `~/.local/share/frame-vrc/launch-options.438100.prev` 恢复原参数） |
| SteamVR 中的 VRChat 单眼渲染分辨率 | `setup`、`resolution N` | 执行 `official`（从备份 `~/.local/share/frame-vrc/resolution.438100.prev` 恢复原分辨率） |

## 新增文件与目录

| 用途 | 路径 |
|---|---|
| 命令行工具与 VR 面板 | `~/.local/lib/frame-vrc`、`~/.local/bin/frame-vrc`、`~/.local/share/applications/frame-vrc.desktop` |
| 独立 Proton 兼容工具副本（约 1.5 GB） | `~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr/` |
| 私有 FEX 运行环境、组件包与配置 | `~/.local/share/frame-vrc/` |
| FEX 安装包缓存 | `~/fex-eac-work/` |
| 眼动追踪桥接共享文件（64 字节） | `~/.local/share/frame-pcvr/gaze/` |
| VRChat 专用 FEX Thunk 配置 | `~/.local/share/Steam/steamapps/compatdata/438100/fex-emu/ThunksDB.json` |

## 未修改的系统组件

以下内容保持原样，不受任何影响：
- 系统目录（`/usr`、`/etc`、`/opt`）、系统固件及其他分区
- Steam 自带的 FEX-Emu、Proton Experimental 原文件、Proton EasyAntiCheat Runtime
- VRChat 游戏本体文件及 Steam 库中的其他游戏

## 卸载

卸载前请先退出 VRChat，并保持 SteamVR 处于运行状态以便自动还原渲染分辨率。

```sh
# 恢复 VRChat 为官方安卓版，移除独立 Proton 副本、私有 FEX 与 frame-vrc 本体（Steam 会自动重启一次以刷新列表）
frame-vrc uninstall

# 彻底卸载：同时清理 ~/.local/share/frame-vrc 与 ~/fex-eac-work 缓存目录
frame-vrc uninstall --purge
```

注：Proton Experimental、EAC Runtime 与 VRChat 游戏本体仍保留在 Steam 中。VRChat 的 Wine 前缀目录（`steamapps/compatdata/438100`）由 Steam 管理，随 Steam 卸载 VRChat 时一并移除。

### 手动还原步骤

若需不通过脚本手动还原：
1. 在 Steam 中打开 **VRChat > 属性 > 兼容性**，将兼容工具改回 **默认（Lepton）**。
2. 在 **VRChat > 属性 > 通用** 中清空启动参数。
3. 删除上方“新增文件与目录”表格中列出的目录。
