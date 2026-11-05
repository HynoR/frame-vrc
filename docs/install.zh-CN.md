# 安装

[English](install.md) · **中文** · [日本語](install.ja.md)

所有操作都在 Frame 上做（Konsole 或 SSH），用普通用户（`steamos`）。不要用 sudo。

## 1. 前提条件（所有方式都要）

1. 在 Steam 里安装 **FEX-Emu**（库 > 工具）、**Proton Experimental** 和 **Proton EasyAntiCheat Runtime**。
   装好 frame-vrc 后，`frame-vrc install-deps` 会为每个缺的项目打开 Steam 的安装确认。
2. **VRChat > 属性 > 兼容性 > Proton Experimental。** 不设的话 Steam 装的是 Android 版。让 Steam 下载 Windows 版。
3. 第一次设置时让头显插着电；头显休眠会断开 SSH。

## 2. 安装 frame-vrc

二选一：

```sh
# a) 从 GitHub 安装：装的是最新的 release（会自己校验 SHA256SUMS）
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash

# b) 用下载好的 frame-vrc-<version>.tar.gz（比如 Discord 上的）：先校验再安装
sha256sum -c --ignore-missing SHA256SUMS        # 在放着压缩包和 SHA256SUMS 的文件夹里
tar xzf frame-vrc-<version>.tar.gz && sh frame-vrc-<version>/install.sh
```

装到 `~/.local/lib/frame-vrc`，链接为 `~/.local/bin/frame-vrc`。如果提示找不到 `frame-vrc`，开一个新的终端或者用 `~/.local/bin/frame-vrc`。

## 一条命令

装好 frame-vrc 后，`frame-vrc setup --resolution 1056` 会自己做完下面的第 1、3、4 部分（前提条件、最新 release 的 FEX 包、切换、启动选项、分辨率、doctor）。
要用别的包就给它地址或文件：`frame-vrc setup ~/Downloads/fex-<version>-steamframe-aarch64.tar.gz`。下面几部分是同样的步骤，一步一步来。

## 3. 获取补丁版 FEX

### 方式 A：用打包好的（推荐）

把 `fex-<version>-steamframe-aarch64.tar.gz` **和**它的 `.sha256` 下载到同一个文件夹，或者直接给 release 地址：

```sh
frame-vrc fetch ~/Downloads/fex-<version>-steamframe-aarch64.tar.gz      # Discord 上下载的文件
frame-vrc fetch https://github.com/.../fex-<version>-steamframe-aarch64.tar.gz   # release 里的文件
```

校验和不对、或者里面的 FEX 跑不起来，`fetch` 都会拒绝，之前的版本留作 `stage.prev`。`-thunk-` 包用法一样（可选，只能桌面模式）。

### 方式 B：在头显上编译

```sh
frame-vrc build              # 4 个线程约 5 分钟，在 ~/fex-eac-work 里占约 2 GB
```

头显会很热（实测最高约 98°C）；到 100°C 会自动停，再运行一次会接着编。插着电、别戴在头上。

### 方式 C：完全手动（不用 frame-vrc）

1. 解压一个包：`mkdir -p ~/fex-eac-work && sha256sum -c fex-*.sha256 && tar xzf fex-*.tar.gz -C ~/fex-eac-work`
   （得到 `~/fex-eac-work/stage`）。
2. 退出所有游戏（FEXServer 不能在运行：`pgrep -x FEXServer` 没有输出）。
3. 用 steamac 的脚本换上（本仓库里也有，`lib/install-fex-tool.sh`），它会先备份 Valve 的文件：
   `sh install-fex-tool.sh`（`status` 看装的是哪个，`restore` 换回 Valve 的）。
4. 把下面的启动选项粘贴到 VRChat > 属性 > 启动选项。
5. 可选的分辨率：`/opt/steamvr/bin/linuxarm64/vrcmd --settings-int steam.app.438100.resolutionOverride`
   （记下原值），然后 `--set-settings-int steam.app.438100.resolutionOverride 1056`（最吃紧时用 720）。

## 4. 切换和设置

```sh
frame-vrc switch eac             # 备份 Valve 的 FEX，装上补丁版
frame-vrc launch-options set     # 通过 Steam 自己的 API 设置；`show` 会打印出来让你自己粘贴
frame-vrc resolution 1056        # 可选；720 只在最吃紧时用；`reset` 恢复原值
frame-vrc doctor
```

frame-vrc 给 `eac` 版本设置的启动选项：

```text
env DXVK_CONFIG="dxgi.maxDeviceMemory = 1024; dxvk.numCompilerThreads = 4" WINEDLLOVERRIDES="tabtip.exe=d" FEX_MAXINST=500 EAC_LAUNCHERDIR=$HOME/.local/share/Steam/steamapps/compatdata/438100/pfx/drive_c/users/steamuser/AppData/Roaming/EasyAntiCheat PROTON_EAC_RUNTIME="$HOME/.local/share/Steam/steamapps/common/Proton EasyAntiCheat Runtime" WINEDEBUG=-all PROTON_USE_XALIA=0 WINE_CPU_TOPOLOGY=16:0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7 %command%
```

各部分的作用：`EAC_LAUNCHERDIR` / `PROTON_EAC_RUNTIME` 让 EAC 用 Valve 的运行时；`WINE_CPU_TOPOLOGY` 报 16 个 CPU（VRChat 按这个数决定线程池大小）；
`DXVK_CONFIG` 报 1 GB 显存、用 4 个着色器编译线程；`tabtip.exe=d` 去掉 Wine 的触屏键盘（约 135 MB）；`FEX_MAXINST=500` 让 FEX 按更小的块翻译，
是 Valve 在 Proton ARM64 上自己用的值（游戏内存约 −0.65 GB、zram 约 −1.9 GB，帧数约 −10%）；`PROTON_USE_XALIA=0` 和 `WINEDEBUG=-all` 关掉无障碍和日志的开销。

`thunk` 版本用 `frame-vrc switch thunk`，启动选项会自动改（Vulkan thunk 开关、驱动修正、`--no-vr`）：见 [thunk-build.zh-CN.md](thunk-build.zh-CN.md)。

## 5. 省内存的 VRChat 设置

VRChat 里（设置，在 VR 里改）：隐藏 Poor / Very Poor 的模型，调低模型下载大小和解压大小上限，减少最多显示的模型数，打开粒子限制，
镜子低画质或关掉。人多的世界里占满内存的是别人的模型。玩之前重启头显，Steam 界面的进程会重新开始，占得少一些。

## 6. 检查是否正常

在 Steam 里以 VR 模式启动 VRChat。第一次进世界约 3 分钟（FEX 在翻译并缓存代码），之后约 20 秒。要记录一局：先运行 `frame-vrc bench` 再启动游戏；
报告在 `~/.local/share/frame-vrc/bench/`，会写明 EAC 有没有启动（`Launcher finished with: 301`、`AntiCheat Session Begin: Success`）。

启动不了的话，在启动选项里加一次 `PROTON_LOG=1`，然后看 `~/steam-438100.log`。

全部撤销：[changes.zh-CN.md](changes.zh-CN.md)。
