# FEX-EAC 在 Frame 上的性能 / 内存科研方向（2026-11-03，只读调查，什么都没改）

背景见 `fex-eac-frame-experiment.md`：VRChat（VR 模式）能跑，瓶颈是内存（Steam+VR 约 7G，游戏 RSS 峰值 4.8G，可用内存最低 534MB，zram 用了 6G）。

## 实测事实

- **为什么整条链都在模拟**：EAC 的 Linux 客户端是 x86-64 `.so`，必须加载进 Proton 进程，所以用不了 Valve 推荐的 Proton ARM64 路线（原生 Wine，FEX 作为 DLL 只翻译游戏代码）。
  现在是 x86 Proton 跑在 FEX 的 Linux 用户态模式下，**Wine、DXVK、Vulkan 驱动也都在模拟**，这是性能和内存开销的根源。
- **Vulkan thunk 没开**：FEX-Emu 工具目录里有 `HostThunks/libvulkan-host.so`、`GuestThunks/libvulkan-guest.so`（Valve 编的，我们替换时没动），`Config.json` 里也写了 thunk 路径；
  但 `compatdata/438100/fex-emu/AppConfig/` 是空的，没设置 ThunksDB，所以 VRChat 用的是 `/usr/share/guestos/fex-mesa/` 里的 x86 Mesa（Turnip），**在模拟运行**。
- 开 thunk 的开关在 `fex-compat-tool`：环境变量 `STEAM_COMPAT_FEX_CONFIG` 里写 `ThunksDB_Vulkan:1`。
  **坑**：一旦设置了这个变量，里面没写 `TSOEnabled:1` / `Multiblock:1` 的话，这两项会被设成 **0**（`fex-compat-tool` 第 63 行），所以要写成 `TSOEnabled:1 Multiblock:1 ThunksDB_Vulkan:1`。
- Valve 还会通过 appinfo 给每个游戏单独设 `STEAM_FEX_TSOENABLED` / `STEAM_FEX_MULTIBLOCK`，说明 Valve 自己也按游戏调 TSO。
- FEX 省内存的选项默认已经打开：`DisableL2Cache=true`、`DynamicL1Cache=true`。补丁版模板开了 `DiskCache`，但**还没找到缓存文件**（位置和效果都没验证）。
- 补丁版 FEX 带 `ProfileSampleHz`（进程内采样，写到 `/dev/shm/fex-<pid>-samples`），可以用来找热点。
- GPU 显存可以按进程读：`/proc/<pid>/fdinfo/<drm fd>` 里有 `drm-total-memory` 等（msm 驱动）。Adreno 和系统共用内存，这部分也算在 16G 里。
- 空闲时 `Shmem` 1.3G，还没拆分清楚是 GPU 缓冲、Wine 还是 FEX 的共享内存。

## 方向（按顺序）

### A. 先测量（不改任何东西，扩展 `frame-vrc bench`）
1. 按进程统计 PSS（`/proc/<pid>/smaps_rollup`），分成游戏 / wineserver / FEXServer / SteamVR。
2. 游戏进程内部：区分 FEX 自己的内存（JIT 代码缓冲、每线程状态）和游戏堆。看 smaps 里 FEX 有没有给映射命名（`[anon:...]`），没有的话按大小区分。
3. GPU 显存：`drm-total-memory`、`drm-resident-memory`（游戏和 vrcompositor 分开看）。
4. 先弄清各部分占比，再决定砍哪一块。

### B. 不改代码，用启动选项就能试、能回退（每项用 bench 做一次对比测试）
1. **Vulkan thunk**（推测是性能上最有希望的一项）：启动选项加 `STEAM_COMPAT_FEX_CONFIG="TSOEnabled:1 Multiblock:1 ThunksDB_Vulkan:1"`。
   换成原生 Turnip，省掉驱动这一层的模拟 CPU 开销和 JIT 内存。
   风险：Valve 的 thunk 是对着 FEX 2607 编的，我们跑的是 2610，接口可能对不上（最坏是游戏崩溃）；EAC 会不会注意到 thunk 库，未知。
2. `WINE_CPU_TOPOLOGY` 16 和 8 对比：线程少了，FEX 每线程的状态也少；但 Mac 上的结论是 4→16 是必需的（不然区域查询会饿死），Frame 有 8 个核，要重新测。
3. 确认 DiskCache 有没有真的在工作：Mac 上第二次进世界从 86 秒降到 32 秒。
4. 游戏这边：VRChat 头像性能限制、显示的头像数量；DXVK 的 `dxgi.maxDeviceMemory`（限制报告给游戏的显存）。

### C. 改 FEX 代码（明天弄的仓库）
1. **用同一版本编 thunk**（`BUILD_THUNKS=ON`，需要 x86-64 编译目标和 rootfs 头文件）：让 B1 的接口对得上，成为安全的选项。
2. 每线程内存：Wine 和 IL2CPP 线程很多，排查 FEX 每个线程分配了什么（信号栈、L1 缓存、CPU 状态、代码缓冲有没有共享）。
3. TSO 开销：骁龙没有硬件 TSO，按模块 / 代码段放宽 TSO（收益大，正确性风险高，要有 eac-tests 覆盖）。
4. EAC 补丁本身的开销（0002 SMC 热页、ptrace 模拟）：用 `ProfileSampleHz` 采样，看它们是不是热点。

### D. 系统层（需要 sudo；不在本研究"只动 ~、不用 sudo"的守则范围内，除非用户明确同意）
zram 的压缩算法和大小、`vm.swappiness`、透明大页（THP）。只列出来，不做。

## 实测记录（2026-11-03，frame-vrc bench，VR 模式）

| 步骤 | 配置 | 结果 |
|---|---|---|
| 1 热缓存基线 | 补丁版 FEX，frame-vrc 启动选项 | 8.8 分钟，105 个采样，进了 2 个世界。从开始采样到进世界约 20 秒（冷缓存第一次约 3 分钟；DiskCache 有效，在 `shadercache/438100/fex-emu/DiskCache/`，837M）。游戏 CPU 平均 282%、峰值 381%；游戏 PSS 平均 4.4G、峰值 5.4G；**游戏 GPU 显存平均 2.2G、峰值 2.4G**；可用内存平均 1.25G、最低 687MB；**zram 满了（7.83G/7.83G）**；温度平均 94°C、峰值 97.8°C |
| 2 Vulkan thunk | 加 `STEAM_COMPAT_FEX_CONFIG="TSOEnabled:1 Multiblock:1 ThunksDB_Vulkan:1"` | **失败**：thunk 确实加载了（`libvulkan-host/guest` 在 4–5 个进程的 maps 里）。~~EAC 还是 `301`~~ **更正**：那个 301 是步骤 1（19:36:34）留下的旧日志，开 thunk 的几次 EAC 启动器都没写日志；但游戏**卡在加载画面**（用户更正：不是黑屏，是一直加载，只能从 SteamVR 结束它），连试 5 次都一样，VRChat 的游戏日志都没生成。推测原因：Valve 的 thunk 是对着 FEX 2607 编的，我们跑的是 2610，接口不匹配（没开 PROTON_LOG，没有崩溃栈）。已恢复步骤 1 的启动选项（备份在 `~/.local/share/frame-vrc/launch-options.438100.step1`） |

结论（当天修正）：~~原因是 2607/2610 接口不匹配~~，这个推测**基本被推翻**。见下面"thunk 版本差异"。

工具改进（当天）：bench 新增游戏 PSS 和 GPU 显存（drm fdinfo）两列。

### thunk 版本差异（2026-11-03，在 Mac 上对 FEX-Emu/FEX 做 partial clone 后 diff）

- Valve 的 FEX `37265b1`（FEX-2607-76）是我们基线 FEX-2610 `14c92681` 的祖先，中间隔了 680 个提交。
- 这 680 个提交里，**FEX 和 thunk 之间的接口没变**（`Source/Tools/LinuxEmulation/Thunks*` 没有改动），所以 Valve 的 thunk 大概率能正常加载进我们的 FEX。
- 变的只有 Vulkan thunk 本身：`27d8d5798` Vulkan 头文件升级、`d96e48e24` 修 vkGetProcAddr 的 bug、`libvulkan_interface.cpp` +368 行、`Host.cpp` +117 行。
- 推测：黑屏是因为 2607 的 Vulkan thunk 不支持 DXVK 或 VR 模式需要的某样东西（扩展 / 函数）。用 2610 重编 thunk 能带上这些新定义，**有可能修好，但不保证**，先看崩溃日志。

## 计划：C1 用 2610 编 thunk（2026-11-03 定）

### 要求：同时保留三个 FEX 版本，随时切换
| 名字 | 内容 | 在哪 |
|---|---|---|
| `valve` | Steam 原版 FEX-2607-76 + Valve 的 thunk | `valve-backup/`（**还要补备份 thunk 目录**） |
| `eac` | steamac 补丁版 eac-2610.5（目前的 `patched`），thunk 用 Valve 原版的 | `stage/` |
| `mod` | 我们的魔改版：eac-2610.5 + 用 2610 编的 thunk（以后的 FEX 改动也放这里） | `stage-mod/` |

thunk 文件不在现有切换的 10 个文件里：`usr/lib/aarch64-linux-gnu/fex-emu/HostThunks/`、`usr/share/fex-emu/GuestThunks*/`、`usr/share/fex-emu/ThunksDB.json`。
`frame-vrc switch` 要扩展成三态，并且**在第一次动 thunk 之前**，先把 Valve 的 thunk 目录备份进 `valve-backup/`。

### 步骤
0. **拿证据**（便宜，先做）：启动选项里 thunk 设置 + `PROTON_LOG=1`，启动一次，看 `~/steam-438100.log` 崩在哪。确认完就恢复启动选项。
1. **宿主端 thunk**（aarch64）：Frame 上有 libclang-cpp、libLLVM、clang 头文件和 clang 的 CMake 配置，但**缺 LLVM 头文件（`/usr/include/llvm`）和 `LLVMConfig.cmake`**。
   用 `pacman -Sp llvm` 拿到和系统同版本的包（llvm 19.1.7-1.1 aarch64）的下载地址，下载后解压到 `~`，不安装。
   **注意**：Frame 的软件源是 Valve 私有源，URL 带 `DO_NOT_SHARE_URL`，不要写进文档或 frame-vrc 仓库；工具里让每台机器自己 `pacman -Sp` 去查。
2. **客户端 thunk**（x86_64，在 Frame 上用 clang `-target x86_64-linux-gnu` 交叉编译）：需要 x86_64 sysroot（glibc 头文件 + libstdc++ 头文件）。
   Frame 上的 `/usr/share/guestos/fex-mesa`（glibc 2.41）只有库，没有头文件；从发行版下载对应版本的 x86_64 开发包，解压到 `~/x86-sysroot`（不装进系统，不用 pacman）。只编 `libvulkan` 一个 thunk。
3. 打包成 `stage-mod`，扩展 `frame-vrc switch`（三态 + 备份 thunk 目录），在机器上来回切换一遍验证能还原。
4. bench 对比：`eac`（步骤 1 基线）vs `mod` + `ThunksDB_Vulkan:1`。

## C1 执行记录（2026-11-03 晚）

- 步骤 0：Valve 2607 thunk + `PROTON_LOG=1`：日志里 thunk 正常挂接（`Linking address ... to host invoker`），随后 `[Vulkan Loader] ERROR: vkDestroyInstance: Invalid instance`。
  和 2610 的 `d96e48e24`（vkGetInstanceProcAddr / vkGetDeviceProcAddr 查询自身时要返回 thunk 自己的函数）对得上；VR 模式下 OpenXR 运行时会通过游戏给的 `vkGetInstanceProcAddr` 建自己的 Vulkan 对象（推测）。日志：`~/fex-eac-work/proton-thunk-valve2607.log`。
- 编译（都在 `~`，系统零改动，不改 FEX 源码）：
  - 宿主端：系统自带 libclang-cpp / libLLVM / clang 头文件；缺的 LLVM 头文件和 cmake 配置来自 `pacman -Sp llvm`（19.1.7-1.1，和系统一致）解压到 `~/fex-eac-work/thunk-deps/llvm`，缺的 .so 链接到 `/usr/lib`。`cmake -DBUILD_THUNKS=ON -DLLVM_DIR=...` 在 `build-mod/`，目标 `vulkan-host-64`。
  - 客户端：`~/x86-sysroot` = Arch 公开归档的 glibc 2.41+r65、linux-api-headers 6.14、gcc 15.1.1+r7、gcc-libs 15.1.1+r7（和 fex-mesa 里的 glibc 2.41 / GLIBCXX_3.4.34 对应），X11/xcb/wayland 头文件软链到系统 `/usr/include`。
    坑：guest 子项目的工具链文件不带 sysroot → 要在 `build-mod/Guest` 里 `cmake -DCMAKE_SYSROOT=~/x86-sysroot .`，然后 `ninja vulkan-guest`。
  - 产物和 Valve 的对比：依赖库一样，glibc 要求都不超过 2.34，导出函数多 70–130 个（2610 新增的接口）。strip 后放进 `~/fex-eac-work/stage-mod`。
- `frame-vrc` 0.2.0：`switch status|valve|eac|mod`，Valve 的 thunk 备份在 `valve-backup/FEX-*/thunks/`；切换时启动选项跟着变（mod 自动加 thunk 开关）。
  实测 eac→mod→valve（10 个文件和基线一致，thunk 和备份一致）→eac→mod 全部正常。

- 步骤 2b（我们自己编的 2610 thunk + PROTON_LOG）：thunk 加载了，**没有再出现 `vkDestroyInstance: Invalid instance`**；但还是没进游戏：游戏进程跑了 2 分钟，CPU 约 1 个核，**GPU 显存一直是 0**，VRChat 没生成游戏日志，**EAC 启动器也没更新日志**（卡在 EAC 启动器完成之前）。
  `WINEDEBUG=-all` 把 Proton 日志压得几乎什么都没有，下次诊断要去掉它。
- bench 的坑：每次启动前面都有一个很短的进程（0.1–0.2 分钟），bench 会跟着它结束；要改成游戏退出后继续等下一个进程。

## 步骤 3：WINE_CPU_TOPOLOGY 16 vs 8（2026-11-03，eac 版本，热缓存）

| | 16（步骤 1 基线） | 8 |
|---|---|---|
| 时长 | 8.8 分钟，2 个世界（负载未记录） | 7.4 分钟，3 个世界：中等负载单人 / 中等负载 3–7 人 / 轻量单人 |
| 游戏 CPU | 平均 282%，峰值 381% | 平均 266%，峰值 409% |
| 游戏 PSS | 平均 4.40G，峰值 5.42G | 平均 4.10G，峰值 5.26G |
| 游戏 GPU 显存 | 平均 2.20G，峰值 2.39G | 平均 1.93G，峰值 2.37G |
| 可用内存 | 平均 1.25G，最低 687MB，12% 的采样低于 1G | 平均 1.43G，最低 **260MB**，35% 的采样低于 1G |
| zram | 满 | 满 |
| 内存压力 PSI full avg300（退出时） | 0.86% | 2.35% |
| 温度 | 平均 94.0°C，峰值 97.8°C | 平均 93.2°C，峰值 96.7°C |
| 进世界 | 正常 | 正常，换世界约 30 秒 |

结论：**看不出明显差别**。PSS 平均少了约 300MB（7%），但最低可用内存和内存压力反而更差，这很可能是第二个世界有 3–7 人造成的；两次的世界不同，没法下结论。
没出现 Mac 上那种 4 核时区域查询饿死的情况。已改回默认的 16。下次要比较就固定同一个世界、同样时长。

## 步骤 4：thunk 诊断（2026-11-05，mod 版本）

工具：`PROTON_LOG=1 WINEDEBUG=+seh,+loaddll,+process VK_LOADER_DEBUG=error,warn,driver`；游戏用 `steam steam://rungameid/438100` 远程启动（首次会弹启动模式选择，用户选了 VR 并记住）；
关游戏用 `steam-launchopts.py stop 438100`（`SteamClient.Apps.TerminateApp`，新增）；容器内视图用 `~/pv-peek.sh`（等 `pv-adverb` 出现后读 `/proc/<pid>/root`，只读）。
注意：FEX 下 wineserver 等 x86 进程的进程名是 `exe`，`pgrep -x wineserver` 匹配不到；`pkill -f <脚本名>` 会把 ssh 自己的 bash 杀掉。

### 原因 1（已绕过）：原生侧 Vulkan loader 找不到驱动
- 现象：`vrclient_init_registry Could not create VK instance, res -9`（VK_ERROR_INCOMPATIBLE_DRIVER），DXVK `Failed to create Vulkan instance`，
  dxgi 抛异常；开着日志时游戏自己退出，关着日志时表现为卡在加载（就是 11-03 的"卡住"）。之前 GPU 显存为 0、EAC 启动器没写日志都是这个原因。
- 原因：pressure-vessel 给容器设的 `VK_DRIVER_FILES`/`VK_ICD_FILENAMES` 只有 x86 的
  `freedreno_icd.x86*.json`，指向 `/run/gfx/main/usr/lib/libvulkan_freedreno.so`。`/run/gfx/main` 是 FEX 给 x86 侧的图形栈
  （宿主机的 `/usr/share/guestos/fex-mesa`），只有模拟侧看得到；thunk 后真正干活的原生 ARM64 loader 打不开 → `Found no drivers!`。
  Valve 自带的 thunk 也是同样失败（所以 11-03 两个版本都卡）。
- 容器里原生驱动在 `/run/host/usr/lib/libvulkan_freedreno.so`，依赖库通过 `overrides/lib/aarch64-linux-gnu` 链到 `/run/host/usr/lib`。
- `VK_ADD_DRIVER_FILES` 会被 pressure-vessel 剥掉（`VK_LOADER_DEBUG` 能传进去），不能用。
- **绕过办法**：FEX 的 `HostEnv` 选项只给原生侧设环境变量（`FEXInterpreter.cpp` 在加载前 `putenv`），`FEX_` 开头的变量能穿过 pressure-vessel：
  启动选项加 `FEX_HOSTENV=VK_DRIVER_FILES=$HOME/.local/share/frame-vrc/vk/freedreno_host.aarch64.json`，
  json 内容就是系统的 `freedreno_icd.aarch64.json`，`library_path` 改成 `/run/host/usr/lib/libvulkan_freedreno.so`。
  之后 loader 找到 `Turnip Adreno (TM) 750`，instance、device、D3D11（feature level 11_0/11_1）都正常。
  剩一个小问题：`VK_LAYER_VALVE_rpo` 报 `Failed to find 'vkGetInstanceProcAddr'` 被跳过（不影响运行）。

### 原因 2（不做，等 Valve/FEX）：VR 提交帧时崩溃
- 现象：Unity 渲染线程在 `IVRCompositor::Submit` 时 native 侧访问违例（地址 0x7fffec230d60 一带，读到垃圾指针 0x007d5fcf8d570a00），
  随后 `vrcompositor_manual.c:2321 Assertion failed: !status`（`VRCLIENT_CALL` 的 unix 调用出错），UnityCrashHandler 启动，游戏退出。
- 没有 `Unknown Vulkan function` 输出，所以不是缺一个函数。更像是 SteamVR 的 x86 vrclient 把 Vulkan 纹理/队列交给 compositor 时，
  某个结构体或句柄过 thunk 边界没正确转换。
- 难度评估：要先多跑几次带插桩的 VR 才能定位到具体结构体，修改会落在 thunkgen 的结构体重打包/句柄规则（`libvulkan_interface.cpp`/`Host.cpp`），
  是 FEX thunk ABI 的核心；改错可能悄悄破坏内存。**不符合"小改动、可逆"的条件，用户决定不做**（FEX 开发者认为这一块应由人来写）。
  可以把上面两条整理成 issue 报给 FEX（未发，需用户同意）。

### 桌面模式 A/B（`--no-vr`，同一个 home 世界，进世界后 180 秒，mod FEX，只差 thunk 开关）
| | thunk 开 | thunk 关 |
|---|---|---|
| 时长 | 5.2 分钟（含加载） | 3.8 分钟 |
| 游戏 CPU | 平均 **232%**，峰值 347% | 平均 256%，峰值 343% |
| 游戏 PSS | 峰值 5900 MB | 峰值 6035 MB |
| 游戏 RSS | 峰值 6047 MB | 峰值 10204 MB（共享映射重复计） |
| GPU 显存 | 峰值 1630 MB | 峰值 1555 MB |
| 可用内存 | 平均 1935 MB，最低 893 | 平均 2445 MB，最低 1129 |
| zram | 峰值 5884 MB | 峰值 6259 MB |
| 温度 | 平均 84.1，峰值 89.6°C | 平均 83.8，峰值 96.3°C |
| EAC / 进世界 | 成功 / 约 25 秒 | 成功 / 正常 |
| 画面（用户在头显里看） | 正常 | 正常，流畅稳定 |

结论：thunk 在桌面模式**能用、画面正常**，CPU 约少 10%；实际内存（PSS）基本没差别。各一次采样，不算定论。
thunk 更大的价值在 VR：见下一节，原生层（注视点渲染）只有 thunk 才用得上，但要等原因 2 修好。

## 其他方向的发现（2026-11-05，不需要改 FEX）
- **注视点渲染**：容器里 `VK_INSTANCE_LAYERS=VK_LAYER_VALVE_rpo:VK_LAYER_VALVE_fdm_injection`（fdm = fragment density map，Frame 的注视点渲染）。
  这两个层只有原生 ARM64 版本（`/usr/share/vulkan/explicit_layer.d`），x86 图形栈（fex-mesa）没有。**所以现在 eac 无 thunk 的方案下 VRChat 没有注视点渲染。**
  thunk 运行时日志里有 `fdm_injection: instance created`。
- **显存上报上限**（见步骤 5 的更正）：容器里有 `VK_VRAM_REPORT_LIMIT_HEAP_SIZE=4096`，但在 eac（无 thunk）下这个 Mesa 层**没生效**，Unity 看到的是真实的 14683 MB；
  4064 MB 只出现在 thunk 运行里（原生层生效）。在 x86 侧能用的是 DXVK 的 `dxgi.maxDeviceMemory`。
- **渲染分辨率**：`~/.config/openvr/config/steamvr.vrsettings` 里 VRChat 是 72 Hz、`resolutionOverride: 1440`（SteamVR 设置界面的最低值）、`framesToThrottle: 1`，
  自动渲染目标倍率 `gpuSpeedRenderTargetScale` 0.64。更低的值见步骤 5。

## 步骤 5：VR 内存 / 帧数（2026-11-05 下午，eac，无 thunk，home 世界单人，进世界后 5 分钟，自动启动和退出）

自动化：`~/ab-vram.sh` / `~/ab-extreme.sh`（`steam://rungameid` 启动，看 output_log 里的 `Successfully joined room`，300 秒后 `steam-launchopts.py stop`，
每轮之间等 70 秒）。用户戴着头显在 home 世界里，报体感。

| 轮 | 改动 | Unity 看到的显存 | 实测/体感帧数 | 游戏 CPU 平均 | PSS 峰值 | GPU 显存峰值 | 可用内存 平均/最低 | zram 峰值 | PSI | 温度峰值 |
|---|---|---|---|---|---|---|---|---|---|---|
| A | 只设 `VK_VRAM_REPORT_LIMIT_HEAP_SIZE=2048` | 14683 MB（上限失效） | 约 20（体感流畅） | 278% | 5685 | 2328 | 1883 / 874 | 7655 | 0.58% | 97.4 |
| B | 默认 | 14683 MB | 约 20（体感间歇卡顿，同时段我在读 smaps，可能是原因） | 289% | 5915 | 2327 | 2025 / 901 | 7224 | 0.52% | 98.2 |
| C | 三个变量都设（ENABLE/DEVICE_ID/HEAP_SIZE=2048） | 14683 MB（x86 侧开不起来） | 约 20（体感同 A） | 292% | 6056 | 2445 | 1961 / 728 | 7602 | 0.60% | 98.2 |
| D | `DXVK_CONFIG="dxgi.maxDeviceMemory = 1024; dxvk.numCompilerThreads = 4"` + `WINEDLLOVERRIDES="tabtip.exe=d"` | **1024 MB** | 约 20（体感可以，没有实质提升） | **241%** | 6050 | 2425 | 2186 / 1082 | 7370 | 0.47% | 96.3 |
| E | D + 分辨率 1080（SteamVR 实际 1056） | 1024 MB | **29**（体感） | 266% | 5774 | **2090** | 2108 / 927 | 7153 | 0.50% | 96.6 |
| F | D + 分辨率 720 | 1024 MB | **35.6**（SteamVR 统计，进世界 2–3 分钟时 1 分钟内）；体感最高 33，GPU 指示从红变黄 | 254% | 5935 | **1786** | **2156 / 1116** | **6511** | **0.35%** | 96.2 |

结论：
- A/B/C 实际都是"没上限"，可以当三个基线样本，数据很一致：home 世界单人就把 zram 用到 7.2–7.7 GB（共 7.8 GB）。
- **DXVK 编译线程 16→4**：CPU 少约 15%。日志里原来是 `Using 16 compiler threads`（因为 `WINE_CPU_TOPOLOGY` 报 16 个 CPU）。
- **tabtip.exe 关掉**：没有这个进程了，约省 135 MB；可用内存平均多约 200 MB。
- **DXVK 报 1 GB 显存**：Unity 生效（`Graphics Memory Size: 1024MB`），home 世界单人纹理少，看不出省内存；人多的世界才可能有效果（待测）。
- **分辨率**：SteamVR 设置界面最低 1440，但 `vrcmd --set-settings-int steam.app.438100.resolutionOverride N` 可以设更低（SteamVR 自己保存，界面里也看得到；
  实际渲染按 32 对齐，1080→1056）。1440→1056→720：帧数约 20→29→36，GPU 显存 2.4→2.1→1.8 GB，zram 峰值 7.4→7.2→6.5 GB。
  画面明显更糊。用户的看法：24 fps 就够，不必把画质压到最低；等注视点渲染能用（需要 thunk，见步骤 4 原因 2），"默认 720 + 注视点区域 1440" 可能是好方案。
- **SteamVR 帧统计**：`vrcmd --stats` 给出这个游戏的累计帧数（`frame_submits`、`frame_presents`、`reprojected_frames`）和每帧平均 CPU/GPU 时间；
  隔一分钟读两次，差值就是实际帧率。1056 时 CPU 约 20 ms、GPU 约 15 ms（72 Hz 预算 13.9 ms）；SteamVR 按 36 或 24 fps 给游戏定步，要稳定 36 每帧得在 27.7 ms 内。
  `frame-vrc bench` 现在每分钟读一次，报告里多一行 SteamVR。
- **测试期间别读游戏的完整 `/proc/<pid>/smaps`**：读大进程的 smaps 要拿它的内存映射锁，会让游戏卡一下（B 轮的卡顿时间对得上）。`smaps_rollup` 每 5 秒一次的采样一直都有，没影响。

### 内存都去哪了（游戏关着时 + 游戏运行时的快照）
- 游戏关着、只开 SteamVR：可用约 8.7 GB（共 16 GB）。最大的是 Steam 客户端界面 `steamwebhelper` ×10：常驻 1.5 GB + zram 里 1.4 GB，
  主要是两个 Chromium 渲染进程（各约 0.7 GB 常驻）；SteamVR 仪表盘 `vrwebhelper` ×8 约 0.3 + 0.4 GB；`XRService` 约 0.2 + 0.4 GB。
  这些在 VR 里都要用，关不掉。头显当时开机 3 天，Chromium 进程会越跑越大：**玩之前重启头显（或 Steam）**可能能省一些（待测）。
- 游戏运行时：VRChat.exe 的 PSS 约 3.3 GB，其中匿名内存 2.4 GB，FEX 自己约 0.75 GB（`FEXAllocator` 344 MB、`FEXMemJIT` 309 MB、`FEXMem_Lookup_L1` 69 MB）。
  Wine 的辅助进程 explorer、winedevice ×2、plugplay、tabtip、services、rpcss、svchost、steam.exe 每个 130–190 MB，**合计约 1.2 GB**；
  每个的大头都是约 135 MB 的 `FEXAllocator`（FEX 内部的 rpmalloc），这些进程本身几乎不干活。这是剩下最大的可省的地方，要先弄清楚 FEX 的分配器为什么每个进程都占这么多。
  （试过在游戏外用 FEX 跑一个 x86 的 `sleep` 来对比，Steam Runtime 的 `files` 目录直接当 rootfs 不行，没测成。）

## 步骤 6：FEX 运行时选项（方案 A，2026-11-05 晚，eac，VR，720，home 世界单人，进世界后 5 分钟）

先查了 Valve 怎么调 FEX（Frame 上的文件）：
- Beat Saber 走 **Proton 11.0 (ARM64)**：Wine/DXVK/驱动原生，只有游戏的 x86 代码用 FEX（进程内 `libarm64ecfex`/`libwow64fex`）。
  它的全局 FEX 配置（`Proton 11.0 (ARM64)/files/share/fex-emu/Config.json`）：`TSOEnabled 1`、`VectorTSOEnabled 0`、`MemcpySetTSOEnabled 0`、
  `HalfBarrierTSOEnabled 1`、**`MaxInst 500`**、`Multiblock 1`、`X87ReducedPrecision 1`。按游戏的 `fex_application_profiles` 只有 Witcher 3 一条。
- x86 路径（VRChat）的 `ConfigTemplate.json` 没设 MaxInst（默认 5000）。TSO/Multiblock 按游戏来自 appinfo 的 `STEAM_FEX_TSOENABLED`/`STEAM_FEX_MULTIBLOCK`；
  **VRChat 运行时这两个都没有，app 配置是空的**，也就是全用 FEX 默认（TSO 全开、MaxInst 5000）。
- FEX 配置层：环境变量层在 app 配置之后加载，所以 VRChat 启动选项里的 `FEX_<选项>=值` 会生效，只影响 VRChat。

| 轮 | 设置 | 结果 | SteamVR fps | 每帧 CPU / GPU | PSS 峰值 | zram 峰值 | 可用内存最低 |
|---|---|---|---|---|---|---|---|
| base | 默认 | 稳定 | 38.8 | 17.6 / 17.5 ms | 6144 | 6395 | 842 |
| a1 | `FEX_MAXINST=500` | 稳定，体感和 F 一样、不卡 | 33.9 | 20.3 / 15.8 ms | 5271 | 4334 | 1290 |
| a3 | `FEX_TSOENABLED=0` | **进世界约 35 秒后崩溃**（Unity 写了崩溃转储；脚本 160 秒后才发现，因为崩溃处理进程还在） | （崩溃后的数据，不算） | | | | |
| a2 | `FEX_HALFBARRIERTSOENABLED=0` | 稳定 | 34.7 | 20.2 / 12.6 ms | 5201 | 5082 | 1641 |

结论：
- **关 TSO 不可用**（VRChat 半分钟内崩溃）；**HalfBarrier 关掉没有收益**。只靠选项省不掉 TSO 的 CPU 开销。
- MaxInst=500 稳定，帧数没提高；内存看起来比 base 好，但 a2 也差不多，而 base 是这批第一轮（zram 里还有之前的东西），先不归因。
  反过来顺序再测一对（a1 在前、base 在后）确认。

反顺序的一对（第二批，a1r 在前、baser 在后，同样设置）：

| 轮 | 设置 | 结果 | SteamVR fps | 每帧 CPU / GPU | PSS 峰值 | zram 峰值 | 可用内存最低 |
|---|---|---|---|---|---|---|---|
| a1r（第 1 轮） | `FEX_MAXINST=500` | 稳定 | 33.9 | 20.1 / 18.9 ms | 5571 | 5182 | 1372 |
| baser（第 2 轮） | 默认 | 稳定 | 36.4 | 18.9 / 15.3 ms | 5991 | 6926 | 959 |

两种顺序结论一致：**MaxInst=500 游戏内存约 −0.65 GB（PSS 平均 5.4 对 6.1 GB）、zram 峰值约 −1.9 GB（4.8 对 6.7 GB）、可用内存最低约 +0.43 GB，
帧数约 −10%（33.9 对约 37.6），每帧 CPU 多约 2 ms**（块更小，块之间跳转更多）。用户决定把它放进默认启动选项（负载高时内存才是瓶颈）。
推荐分辨率改成 1056（720 只在极端情况用）。

## 下一步
1. FEX 每个进程约 135 MB 的 `FEXAllocator`：先弄清是固定开销还是 rpmalloc 缓存（能在游戏外复现最好）；如果是一个小的、可逆的配置改动就试，否则记录下来交给 FEX 开发者。
2. 人多的世界里测 DXVK 1 GB 显存上限有没有效果；VRChat 游戏内设置（隐藏 Poor/Very Poor 模型、模型下载/解压大小上限、最多显示模型数、粒子限制）由用户在 VR 里设，再对比。
3. 重启头显后测一次空闲内存，看 Steam/SteamVR 的 Chromium 进程能省多少。
4. 可选：把步骤 4 的原因 1、2 整理成 FEX issue（需用户同意再发）。

frame-vrc 0.3.0 已经把 D 的三项设置放进默认启动选项，新增 `frame-vrc resolution show|N|reset`（经 vrcmd，第一次改时记下原值），
`switch thunk` 自动加 `FEX_HOSTENV` 驱动修正和 `--no-vr`，`pack`/`fetch` 支持 thunk 版本，`fetch` 也能用本地文件（Discord 下载的）。

## 步骤 7：FEX 代码缓存只给游戏（方案 C，2026-11-05 晚，eac，MaxInst=500，VR，1056）

每个 Wine 辅助进程（services、plugplay、rpcss、winedevice×2、svchost、explorer、steam.exe……）各有约 70 MB 的 `FEXAllocator`。
原因是 FEX 的 DiskCache：`IndexedDB::PopulateIndex` 把 `_idx.foz` 整个读进内存再建表，每个进程一份。
VRChat 的缓存在 `shadercache/438100/fex-emu/DiskCache/`，每套 FEX 设置一份（实验跑多了有 16 个文件、3.1 GB，最大索引 124 MB）。

不改 FEX：用自己的 `FEX_APP_CONFIG`（`lib/fex-vrchat.json`），默认关 DiskCache，`AppOverrides` 只给 `VRChat.exe` 和
`start_protected_game.exe` 打开（Wine 进程的 ProgramName 是 exe 名）。设了 `FEX_APP_CONFIG` 后 fex-compat-tool 不再生成自己的 app 配置，
VRChat 本来就没有 Valve 的 TSO/Multiblock 设置，所以没有损失；thunk 版本要靠那份配置开 thunk，所以不加。

辅助进程（进世界后 60 秒，只读辅助进程的 smaps，不读 VRChat.exe）：

| 轮 | 辅助进程数 | FEXAllocator 合计 | Pss+Swap 合计 | 进世界 | EAC |
|---|---|---|---|---|---|
| 默认 | 14 | 716 MB | 973 MB | 60 秒 | 301 / Session Begin Success |
| C1 | 14 | 66 MB | 333 MB | 60 秒 | 301 / Session Begin Success |

VR 5 分钟（同 a1 的方法）：

| 轮 | fps | 重投影 | 每帧 CPU / GPU | PSS 峰值 | zram 峰值 | 可用内存最低 |
|---|---|---|---|---|---|---|
| a1 | 33.9 | 60% | 20.3 / 15.8 ms | 5271 | 4334 | 1290 |
| a1r | 33.9 | 68% | 20.1 / 18.9 ms | 5571 | 5182 | 1372 |
| c1（作废：中途进了加载页） | 48.8 | 38% | 11.0 / 9.7 ms | 5030 | 5497 | 1384 |
| c1r | 35.3 | 58% | 20.0 / 15.5 ms | 4860 | 4457 | 1229 |

结论：**辅助进程少约 640 MB，帧数不降，进世界时间不变，EAC 正常**。放进 frame-vrc 0.5.3 默认启动选项。
旧实验留下的 DiskCache 文件（别的 FEX 设置的）不会再用到，可以删整个 `fex-emu` 文件夹让它重建。
