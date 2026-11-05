# Building the Vulkan thunk (the `thunk` build)

**English** · [中文](thunk-build.zh-CN.md) · [日本語](thunk-build.ja.md)

The `thunk` build is the `eac` build plus FEX's Vulkan thunk pair, built from the same FEX source:

- `usr/lib/aarch64-linux-gnu/fex-emu/HostThunks/libvulkan-host.so` (aarch64, runs natively)
- `usr/share/fex-emu/GuestThunks/libvulkan-guest.so` (x86-64, loaded by the emulated game in place of `libvulkan.so.1`)

Most people should `frame-vrc fetch` a packed `thunk` build instead. Building it needs extra headers that SteamOS does
not ship. Everything below stays in `~`: no sudo, no pacman installs, nothing in the system changes.
Valve's prebuilt thunks (FEX-2607) do not work with the eac FEX (2610).

## 1. Host side (aarch64)

SteamOS has clang, libclang-cpp and libLLVM, but not the LLVM headers or `LLVMConfig.cmake`.

1. `pacman -Sp llvm` prints the download URL of the `llvm` package that matches the system (19.1.7 on SteamOS 0.4.5).
   Download it and unpack it into `~/fex-eac-work/thunk-deps/llvm` (`bsdtar -xf`). Do not install it.
   The SteamOS package mirror is private: do not post that URL anywhere; let each headset look it up.
2. Link the libraries that the package's CMake exports expect but the headset already has
   (`~/fex-eac-work/thunk-deps/llvm/usr/lib/*.so*` → the same files in `/usr/lib`).
3. Configure a separate build dir next to the eac one:

   ```sh
   cd ~/fex-eac-work/FEX
   cmake -B build-mod -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_THUNKS=ON \
     -DLLVM_DIR=$HOME/fex-eac-work/thunk-deps/llvm/usr/lib/cmake/llvm   # plus the same options lib/build-fex.sh uses
   ninja -C build-mod vulkan-host-64
   ```

## 2. Guest side (x86-64, cross-compiled with clang)

Needs an x86-64 sysroot with glibc and libstdc++ headers matching the guest graphics stack in
`/usr/share/guestos/fex-mesa` (glibc 2.41, GLIBCXX_3.4.34 on SteamOS 0.4.5). The Arch Linux Archive packages that
matched: `glibc 2.41+r65`, `linux-api-headers 6.14`, `gcc 15.1.1+r7`, `gcc-libs 15.1.1+r7`.

1. Unpack them into `~/x86-sysroot`.
2. Link the X11, xcb and wayland headers from `/usr/include` into `~/x86-sysroot/usr/include` (they are
   architecture-independent).
3. The guest sub-project's toolchain file has no sysroot; set it there, then build:

   ```sh
   cd ~/fex-eac-work/FEX/build-mod/Guest && cmake -DCMAKE_SYSROOT=$HOME/x86-sysroot . && ninja vulkan-guest
   ```

## 3. Stage and switch

```sh
cp -a ~/fex-eac-work/stage ~/fex-eac-work/stage-mod
# copy the two built libraries over the same paths in stage-mod (strip them first), then:
echo "thunk: eac stage + Vulkan thunk built from FEX <describe>" > ~/fex-eac-work/stage-mod/MOD.txt
frame-vrc switch thunk && frame-vrc launch-options set
frame-vrc pack thunk     # to share it
```

Check: the built libraries need no newer glibc than 2.34 and link against the same libraries as Valve's copies.

## Why the launch options carry `FEX_HOSTENV`

With the thunk, the native Vulkan loader runs inside Steam's pressure-vessel container. The container's
`VK_DRIVER_FILES` only lists the x86 Turnip at `/run/gfx/main/...`, a path that exists only for the emulated side,
so the native loader finds no driver. frame-vrc writes `~/.local/share/frame-vrc/vk/freedreno_host.aarch64.json`
(the host's `freedreno_icd.aarch64.json` with `library_path` = `/run/host/usr/lib/libvulkan_freedreno.so`) and
passes it with `FEX_HOSTENV=VK_DRIVER_FILES=…`, which FEX sets for the native side only.

## Known issue: VR

In VR, VRChat crashes as it submits its first frame to SteamVR (`IVRCompositor::Submit`, access violation in the
native side, then `vrcompositor_manual.c:2321 Assertion failed: !status`). No "Unknown Vulkan function" is logged, so
it is not a missing entry point; most likely a structure or handle crossing the thunk boundary unconverted. This is
FEX thunk ABI work and is left to the FEX developers. Desktop mode (`--no-vr`) works.
