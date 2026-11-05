# 编译 Vulkan thunk（`thunk` 版本）

[English](thunk-build.md) · **中文** · [日本語](thunk-build.ja.md)

`thunk` 版本 = `eac` 版本 + 用同一份 FEX 源码编的 Vulkan thunk（一对文件）：

- `usr/lib/aarch64-linux-gnu/fex-emu/HostThunks/libvulkan-host.so`（aarch64，原生运行）
- `usr/share/fex-emu/GuestThunks/libvulkan-guest.so`（x86-64，模拟的游戏用它代替 `libvulkan.so.1`）

大多数人直接 `frame-vrc fetch` 别人打包好的 `thunk` 版本就行。自己编需要一些 SteamOS 没带的头文件。下面所有东西都放在 `~`：
不用 sudo、不用 pacman 安装、系统不变。Valve 预编译的 thunk（FEX-2607）和 eac 版 FEX（2610）不能配合使用。

## 1. 宿主端（aarch64）

SteamOS 有 clang、libclang-cpp 和 libLLVM，但没有 LLVM 头文件和 `LLVMConfig.cmake`。

1. `pacman -Sp llvm` 会打印和系统同版本的 `llvm` 包的下载地址（SteamOS 0.4.5 上是 19.1.7）。下载后解压到
   `~/fex-eac-work/thunk-deps/llvm`（`bsdtar -xf`），不要安装。SteamOS 的软件源是私有的：不要把这个地址发到任何地方，让每台头显自己查。
2. 包里 CMake 导出配置需要的库，头显上其实已经有：把 `~/fex-eac-work/thunk-deps/llvm/usr/lib/*.so*` 链接到 `/usr/lib` 里的同名文件。
3. 在 eac 的编译目录旁边另开一个：

   ```sh
   cd ~/fex-eac-work/FEX
   cmake -B build-mod -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_THUNKS=ON \
     -DLLVM_DIR=$HOME/fex-eac-work/thunk-deps/llvm/usr/lib/cmake/llvm   # 再加上 lib/build-fex.sh 用的那些选项
   ninja -C build-mod vulkan-host-64
   ```

## 2. 客户端（x86-64，用 clang 交叉编译）

需要一个 x86-64 sysroot，glibc 和 libstdc++ 头文件要和 `/usr/share/guestos/fex-mesa` 里的客户端图形栈对应
（SteamOS 0.4.5 上是 glibc 2.41、GLIBCXX_3.4.34）。对得上的 Arch Linux Archive 包：`glibc 2.41+r65`、`linux-api-headers 6.14`、
`gcc 15.1.1+r7`、`gcc-libs 15.1.1+r7`。

1. 解压到 `~/x86-sysroot`。
2. 把 `/usr/include` 里的 X11、xcb、wayland 头文件链接进 `~/x86-sysroot/usr/include`（这些和架构无关）。
3. guest 子项目的工具链文件没带 sysroot，要在那里设置，然后编译：

   ```sh
   cd ~/fex-eac-work/FEX/build-mod/Guest && cmake -DCMAKE_SYSROOT=$HOME/x86-sysroot . && ninja vulkan-guest
   ```

## 3. 放进 stage 并切换

```sh
cp -a ~/fex-eac-work/stage ~/fex-eac-work/stage-mod
# 把编好的两个库（先 strip）覆盖到 stage-mod 里的同样路径，然后：
echo "thunk: eac stage + Vulkan thunk built from FEX <describe>" > ~/fex-eac-work/stage-mod/MOD.txt
frame-vrc switch thunk && frame-vrc launch-options set
frame-vrc pack thunk     # 要分享的话
```

检查：编好的库要求的 glibc 不超过 2.34，依赖的库和 Valve 的版本一样。

## 为什么启动选项里有 `FEX_HOSTENV`

开了 thunk 后，原生的 Vulkan loader 在 Steam 的 pressure-vessel 容器里运行。容器的 `VK_DRIVER_FILES` 只列出
`/run/gfx/main/...` 下的 x86 Turnip，这个路径只有模拟侧存在，所以原生 loader 找不到驱动。frame-vrc 会写
`~/.local/share/frame-vrc/vk/freedreno_host.aarch64.json`（宿主机的 `freedreno_icd.aarch64.json`，`library_path` 改成
`/run/host/usr/lib/libvulkan_freedreno.so`），用 `FEX_HOSTENV=VK_DRIVER_FILES=…` 传进去，FEX 只给原生侧设置这个变量。

## 已知问题：VR

在 VR 里，VRChat 把第一帧交给 SteamVR 时崩溃（`IVRCompositor::Submit`，原生侧访问违例，然后
`vrcompositor_manual.c:2321 Assertion failed: !status`）。日志里没有 "Unknown Vulkan function"，所以不是缺函数入口；
最可能是某个结构体或句柄过 thunk 边界时没有转换。这属于 FEX thunk ABI 的工作，留给 FEX 开发者。桌面模式（`--no-vr`）可以用。
