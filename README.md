# frame-vrc

**English** · [中文](README.zh-CN.md) · [日本語](README.ja.md)

> **For tinkerers only.** This is an unofficial research project for people who are comfortable with a Linux shell,
> reading logs and undoing things themselves. If that is not you, please wait for official support. Use at your own
> risk; see the [disclaimer](#disclaimer).

Run the **PC (Windows) version of VRChat**, Easy Anti-Cheat included, standalone on the **Steam Frame**, with the
patched FEX from [StonkGuy/FEX](https://github.com/StonkGuy/FEX) (`eac-*` tags, from
[steamac](https://github.com/StonkGuy/steamac)'s `fex-eac/`). One tool checks the setup, swaps FEX builds with a
backup, sets the launch options and the render resolution, and records comparable benchmarks.

EAC is not bypassed: the stock EAC client from Valve's Proton EasyAntiCheat Runtime runs under emulation; the patched
FEX only makes the emulation exact enough for it.

The goal is **playing VRChat**. Other games with EAC are not a goal and are untested: the patched FEX may or may not run
them, and the settings here are tuned for VRChat.

**Research stage**, one headset verified (SteamOS 0.4.5 vr, November 2026).

## Status

| FEX build (`frame-vrc switch …`) | VR | Desktop (`--no-vr`) |
|---|---|---|
| `valve`: Steam's FEX-Emu as shipped | EAC fails (the reason for the patch; seen on steamac's Mac setup) | same |
| `eac`: steamac's patched FEX | **plays**: EAC OK, enters worlds; ~20 fps at 1440, ~36 fps at 720 | plays |
| `thunk`: `eac` + Vulkan thunk | crashes on the first frame handed to SteamVR (left to FEX/Valve) | plays, ~10% less CPU |

Memory is the limit: SteamVR, Steam's UI and the tracking service take about 7 of the 16 GB, and VRChat fills the
rest plus zram in busy worlds. See [what we found](docs/history.md#findings).

## Quick start (prebuilt FEX)

On the Frame, in Konsole or over SSH, as the normal user (never sudo):

```sh
curl -fsSL https://github.com/HynoR/frame-vrc/releases/latest/download/get.sh | bash   # installs frame-vrc
frame-vrc install-deps                     # Steam asks you to confirm FEX-Emu, Proton Experimental, EAC Runtime
# Steam: VRChat > Properties > Compatibility > Proton Experimental, then let it download the Windows build
frame-vrc fetch https://github.com/HynoR/frame-vrc/releases/download/<tag>/fex-<version>-steamframe-aarch64.tar.gz
frame-vrc switch eac && frame-vrc launch-options set
frame-vrc resolution 720                   # optional: 1056 for a sharper picture, `reset` for yours
frame-vrc doctor                           # everything "ok"? Start VRChat in VR (first world join takes ~3 min)
```

Other ways (a package from Discord, building on the headset, fully by hand): [docs/install.md](docs/install.md).

## Documentation

| | |
|---|---|
| [Install](docs/install.md) | prerequisites; prebuilt, self-built or fully manual install; VRChat settings; checking it works |
| [What changes and how to undo it](docs/changes.md) | every file and setting touched, where it lives, how to restore it, full uninstall |
| [History and findings](docs/history.md) | what was tried, measured and learned, in order; open questions |
| [Building the Vulkan thunk](docs/thunk-build.md) | the optional `thunk` build |
| [Lab notes](docs/research/) | the raw notes (Chinese) |

## Commands

```text
frame-vrc doctor [game]                    check everything, say what to fix
frame-vrc install-deps [game]              ask Steam to install FEX-Emu, Proton Experimental, EAC Runtime
frame-vrc build [--jobs N]                 build the patched FEX on the headset (~5 min, gets hot, stops at 100 C)
frame-vrc fetch URL|FILE                   use a packed build (its .sha256 next to it): check it, stage it
frame-vrc pack [eac|thunk]                 turn your build into a package for others
frame-vrc switch status|valve|eac|thunk    swap Steam's FEX-Emu (Valve's files are backed up first)
frame-vrc launch-options show|set|clear    VRChat's launch options, through Steam's own API
frame-vrc resolution show|N|reset          VRChat's render resolution in SteamVR, below the UI's 1440 minimum
frame-vrc bench [game]                     record CPU, memory, temperature, SteamVR fps and EAC result for one session
frame-vrc uninstall [--purge]              undo everything
```

## Safety

Runs as the normal user, never sudo, writes only under `~`; nothing in the system, firmware or `/persist` changes.
Every change has an undo ([docs/changes.md](docs/changes.md)). Only games that Steam runs through the FEX-Emu
compatibility tool (x86 Proton) are affected.

## Help the research

Share `frame-vrc bench` reports (`~/.local/share/frame-vrc/bench/`) with the **Benchmark report** issue template or on
Discord: same world and length, one change at a time, and say what you changed. Nothing is uploaded automatically.

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

Apache-2.0, see [LICENSE](LICENSE) and [NOTICE](NOTICE).
