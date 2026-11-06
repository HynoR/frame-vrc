# frame-vrc

**English** · [中文](README.zh-CN.md) · [日本語](README.ja.md)

> [!WARNING]
> **Disclaimer — read this first.** frame-vrc is an unofficial project, not made or supported by Valve, VRChat or
> Epic Games. It installs and switches a lot of unofficial, modified components: a patched FEX (x86 emulator), a copy
> of Proton Experimental with a modified DXVK (foveated rendering), and Steam settings changed through Steam's client.
> Using it takes some experience with Linux, the command line and reading logs, and the ability to undo things
> yourself. **If you are not familiar with this, think carefully before running it**, or wait for official support.
> Use at your own risk; see the full [disclaimer](#disclaimer).

Run the **PC (Windows) version of VRChat**, Easy Anti-Cheat included, standalone on the **Steam Frame**, with the
patched FEX from [StonkGuy/FEX](https://github.com/StonkGuy/FEX) (`eac-*` tags, from
[steamac](https://github.com/StonkGuy/steamac)'s `fex-eac/`) and, optionally, foveated rendering (DFR) from
[ji10me/dxvk-steam-frame-dfr](https://github.com/ji10me/dxvk-steam-frame-dfr). One tool switches between the official
and the patched versions, sets the tested options, starts the game and puts everything back.

EAC is not bypassed: the stock EAC client from Valve's Proton EasyAntiCheat Runtime runs under emulation; the patched
FEX only makes the emulation exact enough for it. The goal is **playing VRChat**; other EAC games are untested.

**Research stage**, one headset verified (SteamOS 0.4.5 vr, November 2026).

## Quick start

1. In Steam: install **Proton Experimental** and **Proton EasyAntiCheat Runtime**; set **VRChat > Properties >
   Compatibility > Proton Experimental** and let it download the Windows build. Install **FEX-Emu** too (Library > Tools) if it is missing.
2. Install frame-vrc on the Frame with one line (Konsole or SSH, as the normal user, never sudo). It downloads the
   latest release, checks its SHA-256 and builds the VR panel (about a minute):

```sh
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash
```

3. In the headset: SteamVR dashboard > Library (app list) > **frame-vrc** > **Set up everything** (tap twice), then
   **Launch VRChat**. Or from the command line: `~/.local/bin/frame-vrc setup` and `~/.local/bin/frame-vrc launch`.

Setup applies the recommended settings: the patched FEX, the DFR Proton, render resolution 1920 with DFR, and the
tested options below. It downloads both packages from the latest release and checks their SHA-256. **Steam and SteamVR
restart once** (to list the new Proton): the view goes away for about a minute, then comes back; setup carries on, and
its Log page shows every step. The first world join takes about 3 minutes; later ones about 20 seconds.

**The VR panel** shows what is set, checks the environment (VRChat, Proton Experimental, EAC Runtime, FEX-Emu, launch
options) with a fix button on each problem, and has resolution, DFR, advanced switches, a Monitor page (memory, swap,
CPU per core, temperature, VRChat's memory and CPU) and a Log page. English or Japanese (sidebar button). Setting taps
apply together 1.5 s after the last one. It is not resident: it closes with the dashboard's close button. Every
button runs a `frame-vrc` command.

Back to the official versions: `frame-vrc uninstall`. Details and other ways: [docs/install.md](docs/install.md).

## Switches

| Command | Choices | Default |
|---|---|---|
| `frame-vrc fex` | `valve` Steam's FEX · `eac` the patched FEX (EAC needs it) | eac |
| `frame-vrc proton` | `stock` Proton Experimental · `dfr` its copy with the DFR DXVK (Steam's own copy stays untouched) | dfr |
| `frame-vrc resolution` | `1056` performance · `1440` balanced · `1920` quality; advanced: `1600`, `2160`, any N | 1920 |
| `frame-vrc dfr` | `auto` on from 1440 up · `off` · `def`; advanced: `wide`, `ext` (for 2160 and up) | auto |
| `frame-vrc tune` (advanced) | `cpus 8\|16\|32` · `dxvk-threads N` · `vram MB` · `reset` | 16 · 4 · 1024 |

SteamVR's own settings cover the resolutions in between; 1056 is listed because SteamVR's UI stops at 1440.
`frame-vrc status` shows everything; `frame-vrc doctor` checks everything and says what to fix.

Always on, measured in VR and with no downside worth a switch: FEX's code cache only for VRChat (−640 MB in Wine's
helper processes), FEX blocks of 500 instructions (−0.65 GB game memory, −1.9 GB zram), Wine's touch keyboard off
(−135 MB), the desktop mirror window at 640×480.

## What helps, what does not

- **Helps:** the patched FEX (without it EAC fails), the options above, and **DFR at 1920**: +15–25% fps in a heavy
  world (shader-heavy worlds gain most; light worlds stay within noise). DFR saves GPU time, not memory.
- **Does not:** the Vulkan thunk (crashes in VR), FEX's TSO off (crashes), DFR at 1440, coarser DFR (`ext` is no faster
  than `def` at 1920), resolutions below 1440 except as a last resort.

Memory is the limit: SteamVR, Steam's UI and tracking take about 7 of the 16 GB. Alone in a heavy world at 1920 about
1.5 GB stays free. Numbers: [docs/research/summary.zh-CN.md](docs/research/summary.zh-CN.md).

## Commands

```text
frame-vrc setup [--no-dfr] [--resolution N] [--dfr URL|FILE] [FEX URL|FILE]
frame-vrc panel | status | doctor | launch | update
frame-vrc fex valve|eac             frame-vrc proton stock|dfr [URL|FILE]
frame-vrc resolution 1056|1440|1920 frame-vrc dfr auto|off|def
frame-vrc uninstall [--purge]       back to Valve's FEX, Proton Experimental, your launch options and resolution
advanced: resolution N · dfr wide|ext · tune · install-deps · build · fetch · pack · launch-options · bench
```

## Documentation

| | |
|---|---|
| [Install](docs/install.md) | prerequisites; packages, building it yourself, by hand; checking it works |
| [What changes and how to undo it](docs/changes.md) | every file and setting touched, how to restore it, full uninstall |
| [History](docs/history.md) | what was tried and learned, by version |
| [Research summary](docs/research/summary.zh-CN.md) | measurements (Chinese) |

## Safety

Runs as the normal user, never sudo, writes only under `~`; nothing in the system, firmware or `/persist` changes.
Every change has an undo ([docs/changes.md](docs/changes.md)). Only VRChat's settings and Steam's FEX-Emu tool change;
Proton Experimental itself is never modified (DFR uses a copy).

## Help the research

Share `frame-vrc bench` reports (`~/.local/share/frame-vrc/bench/`) with the **Benchmark report** issue template or on
Discord: same world and length, one change at a time, and say what you changed. Nothing is uploaded automatically.

## Thanks and related projects

- [steamac](https://github.com/StonkGuy/steamac) and [StonkGuy/FEX](https://github.com/StonkGuy/FEX) (FX GAMES): the
  patched FEX that makes EAC work under Proton, and the `fex-eac/` build and install scripts this tool started from.
- [ji10me/dxvk-steam-frame-dfr](https://github.com/ji10me/dxvk-steam-frame-dfr) ([@ji10me](https://x.com/ji10me)),
  building on ReinaS-64892's and poprox24's DFR work: the foveated-rendering DXVK and eye-tracking bridge.
- [FEX-Emu](https://github.com/FEX-Emu/FEX): the x86 emulator everything here runs on.
- Valve: Proton, the Proton EasyAntiCheat Runtime, SteamVR, OpenVR and the FEX-Emu tool for the Steam Frame.
- [DXVK](https://github.com/doitsujin/dxvk) and Mesa's Turnip driver: Direct3D and Vulkan on the Frame's GPU.
- [Decky Loader](https://github.com/SteamDeckHomebrew/decky-loader): the Steam CEF approach used to set launch options.

## Disclaimer

- **Not for beginners.** Expect blurry pictures, low frame rates, long first loads, crashes and the need to read logs.
  If something breaks you should be able to follow [docs/changes.md](docs/changes.md) to undo it yourself.
- **Unofficial.** Not affiliated with or endorsed by Valve, VRChat, Epic Games (Easy Anti-Cheat), FEX-Emu or
  FX GAMES. Running the PC build on the Frame is not supported by any of them, and a Steam, SteamVR, Proton or VRChat
  update can break it at any time.
- **Your account.** EAC is not bypassed or modified, but whether VRChat and EAC accept this setup is their decision.
  Do not modify game files. You are responsible for following VRChat's Terms of Service.
- **Your hardware.** Building and playing run the headset hot (up to ~98 C measured). Keep it on power and ventilated;
  stop if it gets uncomfortable.
- **No warranty.** Provided "as is" under the Apache-2.0 license, without warranty of any kind; the authors are not
  liable for any damage, data loss or account action.

## License

Apache-2.0, see [LICENSE](LICENSE) and [NOTICE](NOTICE). The DFR package carries its own licenses (DXVK and the Frame
additions: zlib; GAZ2 header: MIT; OpenVR: BSD-3-Clause) in `dfr/licenses/`.
