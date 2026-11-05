# 实验：Steam Frame 上用补丁版 FEX 跑 EAC 游戏（VRChat，VR 模式）（2026-11-03 起草并执行：**通过**）

来源：steamac `fex-eac/`（Mac 上已跑通，见 steamac 的 `fex-eac/README.md`）。
steamac 的 guest 就是 Steam Frame 镜像，所以脚本原样可用；本实验验证它在**真机**上是否同样成立。

**目标**：EAC 日志出现 `Launcher finished with: 301`，VRChat 进入世界。

> **更正（2026-11-03）**：实测是在 **VR 模式**下跑的（用户确认），不是桌面模式。下文起草时"只测桌面模式、x86 Proton 接 arm64 OpenXR 推测接不上"的推测**被推翻**：VR 模式可用。跑通只是目标之一，**不是必须**。

**核心守则**（优先级高于目标）：
1. 不坏硬件：全程**不用 sudo**、不碰 `/persist`、sdb/sdc/sdd、温控/频率/电池 sysfs（测试机的设备守则）。
2. 每一步都可回退，像 Android app 一样能干净卸载：每步写明「写了什么 / 怎么验证 / 怎么撤销」。
3. 一步一停：上一步验证通过并记录后，才做下一步。出现「停止条件」立刻停，先回退再分析。
4. 推测与实测分开写，推测标「推测」。

## 已知事实（2026-11-03 只读实测）

| 项 | 值 |
|---|---|
| 系统 | SteamOS 0.4.5 `VARIANT_ID=vr`，BUILD_ID 20261022.6150929，kernel 6.18.0 |
| CPU / 内存 | 8 核大小核，16G，页大小 4096；各核 CPU flags 一致（`TUNE_CPU=native` 编出来的程序在哪个核上都能跑） |
| 工具链 | clang 19.1.7、cmake、ninja、ld.lld、git、python3 齐全 |
| FEX-Emu 工具 | 已装，`FEX-2607-76-g37265b1`，目录结构与 steamac guest 相同 |
| VRChat | 装的是 **Android 版**（`VRChat.apk`，buildid 26309255），兼容工具未设 |
| Proton | 只有 `Proton 11.0 (ARM64)`；**没有** Proton Experimental、EAC Runtime |
| `compatdata/438100` | 已存在，但无 `pfx/.../AppData/Roaming`（内容待第 0 步记录） |
| `ptrace_scope` | 1（推测不影响：EAC 只 trace 子进程，补丁 0001 在 FEX 内模拟 ptrace） |
| `/home` | 216G，剩 140G |

与 Mac 的差异：没有 DXMT（只有 DXVK + Turnip）；**没有硬件 TSO**（推测会比 M2 慢，不影响 EAC 能否通过）；
8 个真核心而不是 4 个 vCPU；~~只测桌面模式（x86 Proton 接 arm64 OpenXR 运行时，推测大概率接不上）~~ 已推翻：VR 模式实测可用。

## 全部改动清单（回退时对照）

| # | 改动 | 位置 | sudo | 撤销 |
|---|---|---|---|---|
| A | 基线记录 | `~/fex-eac-work/baseline/` | 否 | `rm -rf` |
| B | 安装 Proton Experimental、EAC Runtime | Steam 工具库 | 否 | Steam 里卸载 |
| C | VRChat 改用 Proton Experimental（Android 版被替换为 Windows 版） | Steam 设置 + `common/VRChat` | 否 | 兼容性改回默认，重新下载 Android 版 |
| D | 复制 `fex-eac/` | `~/fex-eac/` | 否 | `rm -rf ~/fex-eac` |
| E | 编译 FEX | `~/fex-eac-work/FEX/`、`stage/` | 否 | `rm -rf` |
| F | 替换 FEX-Emu 中 10 个文件（自动备份） | `steamapps/common/FEX-Emu/` | 否 | `install-fex-tool.sh restore` |
| G | VRChat 启动选项 | Steam 设置 | 否 | 清空 |

**执行方式**：所有命令由用户自己在 SSH/Konsole 里跑。头显休眠会断 SSH，编译期间关掉自动休眠或保持接电。

**不做的事**：`guest-tune.sh`、`realism.sh`（需要 sudo，而且是给 steamac VM 写的）；`steamos-readonly`、pacman；改系统配置。

---

## 第 0 步：记录基线（只读，必做）

目的：留一份「原始状态」，最后回退时逐项比对。

```sh
mkdir -p ~/fex-eac-work/baseline && cd ~/fex-eac-work/baseline
S=~/.local/share/Steam/steamapps
cp $S/common/FEX-Emu/VERSIONS.txt fex-VERSIONS.txt
(cd $S/common/FEX-Emu && sha256sum usr/bin/* FEXCompatTool ConfigTemplate.json VERSIONS.txt) > fex-sha256.txt
cp $S/appmanifest_438100.acf .
ls -laR $S/compatdata/438100 > compatdata-438100.txt; du -sh $S/compatdata/438100 >> compatdata-438100.txt
ls $S/common > steam-common.txt
df -h /home > df.txt
cat /etc/os-release > os-release.txt
```

Steam 里截图或手抄：VRChat 的「兼容性」和「启动选项」当前值。

- 验证：`ls ~/fex-eac-work/baseline` 有上面的文件
- 撤销：`rm -rf ~/fex-eac-work/baseline`（最后再删）
- 记录：____

## 第 1 步：Steam 安装 Proton Experimental 和 EAC Runtime

在 Steam「库 → 工具」中安装 **Proton Experimental**、**Proton EasyAntiCheat Runtime**。

- 写入：`steamapps/common/` 下新增两个目录，Steam 自己管理
- 验证：`ls ~/.local/share/Steam/steamapps/common | grep -i -E "proton|easyanti"`
- 撤销：Steam 里卸载这两个工具
- 记录：____

## 第 2 步：VRChat 切换到 Proton Experimental

VRChat → 属性 → 兼容性 → 强制使用 **Proton Experimental**。Steam 会下载 Windows 版，替换 Android 版。

> 这一步会让原生 Android 版暂时不可用（登录状态可能要重新登录）。
> 可选：先**不填启动选项**、不打补丁，用原版 FEX 启动一次，预期 EAC 失败或卡住（Mac 上原版 2607 就是这样）。
> 这样能确认 x86 Proton 在 Frame 上本身能起来，以后出问题好定位。EAC 认证失败只是进不了游戏，没有封号风险。

- 验证：`ls ~/.local/share/Steam/steamapps/common/VRChat` 出现 `VRChat.exe`；启动一次后有 `compatdata/438100/pfx`
- 撤销：兼容性取消勾选 → Steam 重新下载 Android 版；必要时删掉 `compatdata/438100/pfx`（以基线为准，基线里没有的才删）
- 记录（原版 FEX 的表现）：____

## 第 3 步：复制脚本（在 Mac 上执行）

```sh
scp -r <steamac>/fex-eac steamos@<frame-ip>:~/
```

- 验证：Frame 上 `ls ~/fex-eac` 能看到 `build-fex.sh`、`install-fex-tool.sh`
- 撤销：`rm -rf ~/fex-eac`
- 记录：____

## 第 4 步：编译补丁版 FEX（不安装）

准备：头显**接电源、不要戴、放在通风处**；关掉 VR 内的其他应用；剩余空间 > 10G。

```sh
cd ~/fex-eac
JOBS=4 sh build-fex.sh
```

用 `JOBS=4` 而不是 8 个核：降低发热和内存压力，慢一点没关系。
实测（2026-11-03）：约 5 分钟，峰值 97.8°C，`fex-eac-work` 约 1.9G，内存增加约 2G。

编译过程中，可以在另一个 SSH 里只读查看温度（只读，不改任何 thermal 设置）：

```sh
watch -n 10 'for z in /sys/class/thermal/thermal_zone*; do echo "$(cat $z/type) $(cat $z/temp) trip=$(cat $z/trip_point_0_temp 2>/dev/null)"; done | sort -k2 -n | tail -3'
```

**停止条件**：温度持续接近 trip 点、系统明显卡顿或自动降频提示、`/home` 剩余空间 < 5G → `Ctrl-C` 停止编译。
编译中断不影响系统，以后重跑会接着编。

- 写入：只有 `~/fex-eac-work/FEX`、`~/fex-eac-work/stage`
- 验证：最后一行输出 `staged: .../stage (eac-2610.5)`；`cat ~/fex-eac-work/stage/VERSIONS.txt`
- 撤销：`rm -rf ~/fex-eac-work/FEX ~/fex-eac-work/stage`
- 记录（耗时、最高温度）：____

**到这一步为止，Steam 和所有游戏都没被改动。**

## 第 5 步：替换为补丁版 FEX（第一处真正生效的改动）

先在 Steam 里退出所有游戏。

```sh
cd ~/fex-eac
sh install-fex-tool.sh status     # 记录替换前
sh install-fex-tool.sh            # 先备份 Valve 原版到 ~/fex-eac-work/valve-backup/，再替换 10 个文件
sh install-fex-tool.sh status     # installed 应为 eac-2610.5 对应的版本
ls ~/fex-eac-work/valve-backup/   # 备份必须存在：FEX-2607-76-g37265b1
```

- 写入：`steamapps/common/FEX-Emu/` 下 7 个程序 + `FEXCompatTool`、`VERSIONS.txt`、`ConfigTemplate.json`
- 影响范围：只影响走 FEX-Emu 兼容工具的 x86 游戏；Proton ARM64 游戏不受影响
- 验证：`status` 输出与预期一致；用之前能玩的其他 x86 游戏（如果有的话）抽查一个，确认还能启动
- 撤销：关掉游戏 → `sh install-fex-tool.sh restore` → `(cd ~/.local/share/Steam/steamapps/common/FEX-Emu && sha256sum -c ~/fex-eac-work/baseline/fex-sha256.txt)` 全部显示 OK
- **先演练一次回退**：install → restore → 对照基线 sha256 → 再 install。确认回退可靠，再往下做
- 记录：____

## 第 6 步：填启动选项，启动 VRChat（实测为 VR 模式）

VRChat → 属性 → 启动选项（8 核版 topology）：

```
env EAC_LAUNCHERDIR=$HOME/.local/share/Steam/steamapps/compatdata/438100/pfx/drive_c/users/steamuser/AppData/Roaming/EasyAntiCheat PROTON_EAC_RUNTIME="$HOME/.local/share/Steam/steamapps/common/Proton EasyAntiCheat Runtime" WINEDEBUG=-all PROTON_USE_XALIA=0 WINE_CPU_TOPOLOGY=16:0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7 %command%
```

第一次启动需要编译着色器，耐心等几分钟。

- 成功判据：EAC 日志 `Launcher finished with: 301`；VRChat 日志 `AntiCheat Session Begin: Success`、`Finished entering world`
- 失败对照：`210 'Unexpected error. (#1)'` → 已安装的不是补丁版，检查 `status`；一直黑屏 → 收集 `~/.local/share/Steam/logs/` 和 Proton 日志（启动选项前加 `PROTON_LOG=1`，日志在 `~/steam-438100.log`）
- 撤销：清空启动选项（或 `python3 -I ~/fex-eac-work/steam-launchopts.py clear`）
- 记录（EAC 结果、进世界耗时、帧率主观感受、温度）：____

**停止条件**：头显异常发热、系统卡死需要强制重启、SteamVR/系统 UI 出现异常 → 立即停止，走「完整回退」。

---

## 完整回退（恢复到第 0 步的状态）

按顺序，倒着来：

1. 清空 VRChat 启动选项（G）
2. 关掉游戏 → `cd ~/fex-eac && sh install-fex-tool.sh restore` → 对照基线 sha256 全部 OK（F）
3. VRChat 兼容性取消勾选 → 等 Steam 重新下载 Android 版，确认 `VRChat.apk` 回来了（C）
4. VRChat 不做备份（用户决定，2026-11-03）：环境由 Steam 管理，在 Steam 里卸载 VRChat 再重新安装即可，`compatdata/438100` 会随之重建
5. Steam 里卸载 Proton Experimental、Proton EasyAntiCheat Runtime（B）
6. 先打开 Android 版 VRChat 确认能正常用，再 `rm -rf ~/fex-eac ~/fex-eac-work`（D、E、A；`valve-backup` 和基线也在里面，**第 2 步确认 OK 后**才删）
7. 验证：`ls ~/.local/share/Steam/steamapps/common` 与 `baseline/steam-common.txt` 一致（基线已删的话，以记录为准）

## 只回退补丁版、保留其他（随时切换）

**日常用这个**（2026-11-03 起）：`~/fex-eac/fex-switch.sh`（源码 `docs/research/fex-switch-0.1.sh`，已被 `frame-vrc switch` 取代）

```sh
~/fex-eac/fex-switch.sh status    # valve / patched / unknown
~/fex-eac/fex-switch.sh patched   # 换成补丁版（复用 stage，不重新编译）
~/fex-eac/fex-switch.sh valve     # 换回 Valve 原版，并和基线 sha256 比对
```

它包了一层 `install-fex-tool.sh`，额外做三件事：FEXServer 在运行时拒绝切换（原脚本的 restore 不检查这个）；切完以后核对结果；
如果当前装的既不是基线原版也不是补丁版（Steam 更新过），就停下来，提示先用新的 `BACKUP=` 备份，不会拿旧备份去降级。
已在机器上实测：status → patched（已经是，不动）→ valve（和基线一致）→ patched。`unknown` 分支和"游戏运行中拒绝"两种情况没有实测。

原始命令（脚本出问题时再用）：

```sh
sh install-fex-tool.sh restore   # 切回 Valve 原版
sh install-fex-tool.sh           # 切回补丁版（复用 stage，不重新编译）
```

Steam 更新 FEX-Emu 后，补丁版会被覆盖：先跑 `status` 看情况；想重新装补丁版时，用 `BACKUP=~/fex-eac-work/valve-backup-<新版本>` 另存一份新的 Valve 版。
不要拿旧的 2607 备份去覆盖新版本。

## 结论与基准（2026-11-03）

**结论**：可行。Steam Frame 上原样使用 steamac `fex-eac/`（补丁版 FEX `eac-2610.5`），VRChat（Windows 版、Proton Experimental、**VR 模式**）
通过 EAC 并进入世界；换世界时 EAC 会话结束/重新开始都成功。瓶颈是**内存**，CPU 还有余量；温度长时间在 95°C 左右，但没到上限。

| 项 | 结果 |
|---|---|
| EAC | `301` ×1，`Session Begin: Success` ×2，`Session End: Success` ×1，`Finished entering world` ×2；没有 `210`、`null client` |
| 进世界 | 从启动算起约 3 分钟内（第一次，含编译着色器） |
| 游戏 CPU | 平均 264%，峰值 419%（8 核满载为 800%）；系统 CPU 平均 69% |
| 游戏内存（RSS） | 峰值 4.8G（一部分会被换到 zram，所以 RSS 会往下掉） |
| 系统可用内存 | 平均 1.9G，**最低 534MB**；207 个采样里 3 个低于 1G |
| zram swap | 游戏中最多用了 6.0G / 7.8G；退出后还剩 3.3G 没换回来 |
| 内存压力 PSI | full avg300 ≈ 0.8%（偶尔短暂卡顿） |
| OOM | 没有杀进程 |
| 温度 | 平均 94.7°C，峰值 97.1°C（trip 110°C） |
| GPU | 一直在最高频率 903MHz（只能读频率，读不到占用率） |
| 不跑游戏时（Steam + VR） | 已用约 7.0G，可用 8.7G |
| 编译 FEX | `JOBS=4` 约 5 分钟，峰值 97.8°C，占用约 1.9G 磁盘 |

**和 Mac（M2 16G）比**：同一套 FEX 和启动选项都成立；Frame 没有硬件 TSO、内存要和 VR 运行时共用，可用余量明显更小。

**留在机器上的东西**（截至实验结束）：补丁版 FEX 已装（切换和回退见上文）；VRChat 启动选项已设置；
`~/fex-eac`、`~/fex-eac-work/`（含 `FEX/` 源码 1.85G、`stage/`、`valve-backup/`、`baseline/`、`steam-launchopts.py`、`game-bench.py` 及日志）、`~/vrchat-launch-options.txt`。

## 执行记录

| 日期 | 步骤 | 结果 | 备注 |
|---|---|---|---|
| 2026-11-03 | 0 基线 | 完成 | `~/fex-eac-work/baseline/`：FEX-2607-76 的 10 个文件 sha256、steam-common、df、os-release（VRChat 按用户决定不备份） |
| 2026-11-03 | 1 Proton | 部分完成 | Proton Experimental 已在；**Proton EasyAntiCheat Runtime 未装** |
| 2026-11-03 | 2 VRChat | 完成（用户操作） | Windows 版 buildid 26309255，`start_protected_game.exe`、`EasyAntiCheat/` 存在；还没启动过 |
| 2026-11-03 | 3 复制 | 完成 | 5 个文件 sha256 与源文件一致。坑：macOS tar 会夹带 `._*`，下次加 `COPYFILE_DISABLE=1` |
| 2026-11-03 | 4 编译 | 完成 | `JOBS=4`，18:40:51 → 18:45:44，**约 4 分 53 秒**（含 clone 子模块）；commit `6effd23` 与脚本里固定的版本一致，describe `eac-2610.3-45-g6effd23`。磁盘：`FEX/` 约 1.85G（External 1.4G、.git 342M、build-steam 70M），`stage/` 13M。内存：编译前已用约 7G（Steam/VR），采样到的峰值 8.8G / 15.3G。负载峰值 8.5（8 核）。温度：开始前 76–81°C，峰值 **97.8°C**，结束后 83.6°C；没触发 100°C 自动停止。注：bench.log 采样只覆盖最后 40 秒，内存和负载峰值偏低；温度来自每分钟一次的监控 |
| 2026-11-03 | 5 替换 | 完成 | 没有游戏或 FEXServer 在跑；install 成功，Valve 原版备份到 `valve-backup/FEX-2607-76-g37265b1`；**回退演练**：restore 后 `sha256sum -c` 基线 10/10 OK；再次 install → `installed: eac-2610.3-45-g6effd23` |
| 2026-11-03 | 6 启动选项 | 已设置 | 没让用户在 VR 里粘贴：通过 Steam CEF 调试口（127.0.0.1:8080，SharedJSContext）调用 `SteamClient.Apps.SetAppLaunchOptions`，脚本 `~/fex-eac-work/steam-launchopts.py`（只用标准库）。原值为空；Steam 自己写进 localconfig.vdf，与 `~/vrchat-launch-options.txt` 逐字节一致（319 字节）。坑：`appDetailsStore...strLaunchOptions` 读回空字符串（推测是 UI 缓存），要以 localconfig.vdf 为准 |
| 2026-11-03 | 6 启动 VRChat | **通过** | **VR 模式**。EAC `Launcher finished with: 301`、`AntiCheat Session Begin: Success`（约 18:53），`Finished entering world`（18:54 前后，从启动算起约 3 分钟内）。启动和进世界阶段：游戏 CPU 156–328%（8 核满载为 800%），游戏内存 4.1→4.8G，系统可用内存 2.5→**1.07G**，温度 85→94°C。采样：`~/fex-eac-work/game-bench.py`，日志 `game-bench.log` |
| 2026-11-03 | 6 游戏基线 | 18:52:50–18:58:16，66 个采样，含进世界和简单游玩 | 游戏 CPU 平均 224%、峰值 384%（8 核满载为 800%）；系统 CPU 平均 69%。游戏内存（RSS）峰值 4.8G，之后降到 3.0G（被换到 zram，不是游戏自己释放）。系统可用内存最低 **534MB**，平均 1.9G；**zram swap 用了 6.0G / 7.8G**；内存压力 PSI full avg300 0.86%（偶尔短暂卡顿）。没有 OOM 杀进程（内核日志里两条 OOM 都是 18:31 休眠/唤醒时的提示）。温度峰值 97.1°C，持续在 94–95°C。GPU 一直跑在最高频率 903MHz（只有频率可读，读不到占用率） |
| 2026-11-03 | 6 全程汇总 | 18:52:45–19:10:05，17.2 分钟，207 个采样；游戏正常退出 | 见下方「结论与基准」 |
