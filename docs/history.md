# History

**English** · [中文](history.zh-CN.md) · [日本語](history.ja.md)

One Steam Frame (SteamOS 0.4.5 vr, Snapdragon 8 Gen 3, 16 GB), VRChat Windows build on Proton Experimental.
Measurements: [research/summary.zh-CN.md](research/summary.zh-CN.md) (Chinese).

## Versions

| Version | Date | What changed |
|---|---|---|
| 0.6.1 | 2026-11-06 | DFR on a new install: the eye-tracking bridge's folder is created first (without it the bridge could not write gaze data and DFR stayed at full rate). |
| 0.6.0 | 2026-11-06 | Foveated rendering (DFR): `proton stock\|dfr` (a copy of Proton Experimental with ji10me's DFR DXVK), `dfr auto\|off\|def` (auto = on from 1600), the eye-tracking bridge started with the game (`lib/dfr-run`). Recommended setup: 1920 + DFR. Switches simplified (`fex`, `proton`, `resolution`, `dfr`, `tune`), settings in one file, `status` and `launch`. Desktop mirror window 640×480 by default. Vulkan thunk build removed. |
| 0.5.3 | 2026-11-05 | FEX's code cache only for VRChat and the EAC launcher: Wine helpers ~640 MB less, no fps loss. |
| 0.5.2 | 2026-11-05 | `FEX_MAXINST=500` by default (−0.65 GB game memory, −1.9 GB zram, ~−10% fps). Fixes from a clean-install test. |
| 0.5.0–0.5.1 | 2026-11-05 | `setup` does the whole install; one-line installer (`get.sh`). |
| 0.4.0 | 2026-11-05 | Renamed from frame-eac: the point is PC VRChat, not EAC games in general. |
| 0.3.0 | 2026-11-05 | DXVK 1 GB VRAM report + 4 compiler threads (−15% CPU), tabtip off (−135 MB); `resolution`; SteamVR fps in benchmarks; docs in three languages. |
| 0.1–0.2 | 2026-11-03 | First run: steamac's patched FEX built on the headset; EAC `301` / `Session Begin: Success`; VRChat plays in VR. |

## Findings in short

- **Works:** the patched FEX (EAC fails without it); the default options above; DFR at 1920 (+15–25% fps in a
  shader-heavy world, within noise in a light one; saves GPU time, not memory). DFR's DLLs must go in Proton's own DXVK
  folder: next to `VRChat.exe` the game crashes in Wine's loader.
- **Does not:** the Vulkan thunk (crashes in VR when SteamVR's x86 client submits a frame; desktop only, −10% CPU);
  FEX TSO off (crash in ~35 s); HalfBarrier TSO off (no gain); Mesa's VRAM cap (native side only); DFR at 1440 (no gain);
  coarser DFR (`ext` no faster than `def` at 1920).
- **Limits:** memory (SteamVR, Steam's UI and tracking take ~7 of 16 GB) and CPU (FEX translating x86: ~20 ms per frame
  at 1056, ~35–40 ms at 1920 in a heavy world). SteamVR paces the game at half or a third of 72 Hz.
- **Measuring:** reading a game's full `/proc/<pid>/smaps` makes it stutter (the sampler uses `smaps_rollup`); turn the
  headset's sleep off for unattended runs (standby pauses rendering and stalls logins).

## Open questions

1. Memory in busy worlds at 1920 (alone in a heavy world ~1.5 GB stays free).
2. Hardware video decoding for world video players (now software, emulated).
3. How much a headset restart frees in Steam's UI processes.
