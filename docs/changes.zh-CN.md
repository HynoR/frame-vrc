# 改了什么、怎么恢复

[English](changes.md) · **中文** · [日本語](changes.ja.md)

frame-vrc 只写你的用户目录。下面是它动过的全部东西。

## Steam 的文件里改了什么

| 内容 | 位置 | 由谁改 | 恢复 |
|---|---|---|---|
| FEX-Emu：7 个程序（`FEX`、`FEXBash`、`FEXGetConfig`、`FEXOfflineCompiler`、`FEXServer`、`FEXServerManager`、`FEXpidof`）和 `FEXCompatTool`、`VERSIONS.txt`、`ConfigTemplate.json` | `~/.local/share/Steam/steamapps/common/FEX-Emu/` | `switch eac` / `switch thunk` | `frame-vrc switch valve`（从备份拷回并逐个比对） |
| FEX-Emu 的 Vulkan thunk（只有 `thunk` 版本）：`usr/lib/aarch64-linux-gnu/fex-emu/HostThunks/libvulkan-host.so`、`usr/share/fex-emu/GuestThunks/libvulkan-guest.so` | 同上 | `switch thunk` | `switch eac` 或 `switch valve`（Valve 的原件在备份里） |

万一备份丢了：Steam > 库 > 工具 > FEX-Emu > 卸载，再装一次，Steam 会装回 Valve 的文件。Steam 更新 FEX-Emu 时会覆盖补丁版：
再运行一次 `frame-vrc switch eac`（新的 Valve 版本会被备份，旧备份留作 `valve-backup.old-<日期>`）。

## Steam / SteamVR 设置里改了什么

| 内容 | 位置 | 由谁改 | 恢复 |
|---|---|---|---|
| VRChat 启动选项 | Steam（存在 `~/.local/share/Steam/userdata/*/config/localconfig.vdf`） | `launch-options set`、`switch`（只在是 frame-vrc 设置的时候） | `frame-vrc launch-options clear`（恢复你原来的，原值存在 `~/.local/share/frame-vrc/launch-options.438100.prev`），或者在 VRChat > 属性里删掉 |
| VRChat 渲染分辨率 | SteamVR（`~/.config/openvr/config/steamvr.vrsettings`，`steam.app.438100` > `resolutionOverride`） | `resolution N`（通过 SteamVR 的 `vrcmd`） | `frame-vrc resolution reset`（原值存在 `~/.local/share/frame-vrc/resolution.438100.prev`），或者 SteamVR > 设置 > 视频 > 按应用设置 |

## frame-vrc 创建的

| 内容 | 位置 | 怎么删 |
|---|---|---|
| 工具本身 | `~/.local/lib/frame-vrc`、`~/.local/bin/frame-vrc` | `frame-vrc uninstall` |
| **Valve 版 FEX 的备份**（留着） | `~/fex-eac-work/valve-backup/FEX-<版本>/`（+ `thunks/`） | `uninstall --purge`，只在已经换回 Valve 版后才删 |
| 编译产物和源码 | `~/fex-eac-work/`（`stage`、`stage-mod`、`*.prev`、`FEX`、`build-*`、`build.log`） | `uninstall --purge` |
| thunk 用的驱动清单 | `~/.local/share/frame-vrc/vk/freedreno_host.aarch64.json` | `uninstall --purge` |
| 基准报告、打包文件 | `~/.local/share/frame-vrc/bench/`、`~/.local/share/frame-vrc/dist/` | `uninstall --purge`，或直接删 |

## 玩游戏时产生的（Steam、FEX、Proton 产生，不是 frame-vrc）

| 内容 | 位置 | 删除 |
|---|---|---|
| FEX 代码缓存（玩几次后约 0.8 GB；让之后启动变快） | `~/.local/share/Steam/steamapps/shadercache/438100/fex-emu/` | 删掉文件夹即可，会重新生成 |
| VRChat 的 Wine 前缀（EAC 文件、游戏设置、缓存） | `~/.local/share/Steam/steamapps/compatdata/438100/` | 在 Steam 里卸载 VRChat |

## 不会动的

系统（`/usr`、`/etc`、`/opt`）、固件、`/persist`、其他分区、Proton、EAC Runtime、VRChat 的游戏文件、其他游戏。原生游戏和 Proton ARM64 游戏不用 FEX。

## 完整卸载

先退出游戏（FEX 不能在运行），SteamVR 开着（分辨率恢复要用）：

```sh
frame-vrc uninstall           # 换回 Valve 的 FEX、清掉启动选项、恢复分辨率、删掉工具；保留编译产物、备份和报告
frame-vrc uninstall --purge   # 同时删掉 ~/fex-eac-work 和 ~/.local/share/frame-vrc
```

Proton Experimental、EAC Runtime 和 VRChat 留在 Steam 里；把 VRChat 的兼容性改回默认就能重新用 Android 版。

### 手动（用方式 C 装的）

```sh
sh install-fex-tool.sh restore                       # 从 ~/fex-eac-work/valve-backup 换回 Valve 的 FEX
/opt/steamvr/bin/linuxarm64/vrcmd --set-settings-int steam.app.438100.resolutionOverride <原值>
rm -rf ~/fex-eac-work                                # 先用 `sh install-fex-tool.sh status` 确认
```

再在 VRChat > 属性里删掉启动选项。
