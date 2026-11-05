# History and findings

**English** · [中文](history.zh-CN.md) · [日本語](history.ja.md)

One Steam Frame (SteamOS 0.4.5 vr, Snapdragon 8 Gen 3, 16 GB), VRChat Windows build on Proton Experimental.
Raw notes: [research/](research/) (Chinese).

## Timeline

### 2026-11-03: VRChat with EAC runs in VR

- steamac's `fex-eac` (patched FEX for EAC under Proton, proven on a Mac) built on the headset: ~5 min with 4 jobs,
  ~2 GB, up to ~98 C. Swapped into Steam's FEX-Emu with a backup of Valve's files; every step had an undo.
- Launch options set through Steam's own API (CEF debug port) instead of typing them in VR.
- Result: EAC `Launcher finished with: 301`, `AntiCheat Session Begin: Success`, worlds entered **in VR**. First join
  ~3 min (FEX code cache cold), later ~20 s.
- Baseline (VR, 1440): game CPU ~280% (100% = one core), game memory (PSS) 4.4 GB average / 5.4 GB peak, GPU memory
  ~2.2 GB, available memory down to ~0.7 GB, zram full, ~94 C.
- Vulkan thunks (native Vulkan instead of emulated): both Valve's prebuilt thunk and one built from the eac FEX left
  the game stuck while loading.
- `WINE_CPU_TOPOLOGY` 16 vs 8: no clear difference (different worlds).
- frame-vrc 0.1–0.2: switch between Valve / eac / thunk builds, launch options, benchmark sampler.

### 2026-11-05: thunk diagnosis, desktop mode, VR memory and frame rate

- **Thunk cause 1, fixed:** the native Vulkan loader got the container's x86 driver list (a path only the emulated
  side sees) and found no driver. `FEX_HOSTENV=VK_DRIVER_FILES=<json>` points it at the host's Turnip.
- **Thunk cause 2, left to FEX/Valve:** in VR the game crashes as SteamVR's x86 vrclient submits the first frame
  through the thunk. Fixing it means changing FEX's thunk ABI code.
- **Desktop mode works** with and without the thunk; picture correct; the thunk saved ~10% CPU, no memory.
- **VR memory and frame rate** (home world, alone, 5 min each, automated):

  | Run | Change | fps | game GPU memory | zram peak | lowest available |
  |---|---|---|---|---|---|
  | A–C | baseline (VRAM cap experiments that turned out inactive) | ~20 | ~2.4 GB | ~7.5 GB | ~0.8 GB |
  | D | DXVK 1 GB VRAM + 4 compiler threads, tabtip off | ~20, CPU −15% | 2.4 GB | 7.4 GB | 1.1 GB |
  | E | D + resolution 1056 | 29 | 2.1 GB | 7.2 GB | 0.9 GB |
  | F | D + resolution 720 | 36 (SteamVR stats) | 1.8 GB | 6.5 GB | 1.1 GB |

- frame-vrc 0.5.2: `setup` and `switch` no longer stop silently on a new install (no Valve backup yet); `doctor` and
  `resolution` no longer stop silently without Steam's config or SteamVR. Found by the clean-install test.
- The installer (`get.sh`) is fetched from the repository instead of the release, so fixes to it need no new release;
  it still installs only released versions.
- frame-vrc 0.5.1: the one-line installer (`get.sh`) no longer exits with an error after a successful install.
- frame-vrc 0.5.0: `setup` does the whole install in one command; `switch` also fills in empty launch options.
- Renamed from frame-eac to **frame-vrc** (0.4.0): the point is playing PC VRChat, not EAC games in general.
- frame-vrc 0.3.0: D's settings by default, `resolution` command, SteamVR fps in benchmarks, thunk build packaging,
  `fetch` from a file, docs in three languages.

## Findings

- **Render resolution is the biggest lever.** SteamVR's settings stop at 1440; `vrcmd` (and `frame-vrc resolution`)
  goes lower; SteamVR rounds to multiples of 32 (1080 → 1056).
- **CPU and GPU are both close to the limit.** At 1056: ~20 ms CPU and ~15 ms GPU per frame; SteamVR paces the game
  at 36 or 24 fps (27.7 ms needed for 36). The CPU time is mostly FEX translating x86 code.
- **VRAM the game sees:** Steam's Mesa VRAM cap only works on the native side; under `eac` Unity sees the real
  14683 MB. DXVK's `dxgi.maxDeviceMemory` does work.
- **DXVK compiler threads** follow the reported CPU count (16); 4 cut CPU by ~15%.
- **Foveated rendering:** SteamOS's `VK_LAYER_VALVE_fdm_injection` exists only for aarch64, so the emulated game gets
  none. With the thunk it loads; that is why the VR thunk crash matters.
- **Where memory goes:** with only SteamVR running ~8.7 of 16 GB are free. Steam's UI (~1.5 GB + 1.4 GB in zram),
  the SteamVR dashboard (~0.7 GB) and XRService (~0.7 GB) are needed in VR. In the game, every Wine helper process
  carries ~135 MB of FEX allocator, ~1.2 GB in all.
- **Measuring:** reading a game's full `/proc/<pid>/smaps` makes it stutter; frame-vrc's sampler uses `smaps_rollup`.
  `vrcmd --stats` gives the game's frame counts and CPU/GPU time per frame.

## Open questions

1. The ~135 MB of FEX allocator in every Wine helper process: fixed cost or allocator cache? A small, reversible fix
   would be worth trying; otherwise report it to FEX.
2. The 1 GB VRAM report and VRChat's avatar limits in busy worlds.
3. How much a headset restart frees in Steam's UI processes.
4. The VR thunk crash (upstream). An OpenVR-to-OpenXR layer (e.g. xrizer) in place of SteamVR's x86 vrclient might
   avoid it; untested, and EAC rules out changing game files.
