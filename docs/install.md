# Installation & CLI Reference

**English** · [中文](install.zh-CN.md) · [日本語](install.ja.md)

For an illustrated walkthrough, see the [Step-by-Step Tutorial](tutorial.md). Run all commands on the Steam Frame (in Konsole or over SSH) as the normal user `steamos`. **Never use `sudo`.**

## Install frame-vrc

### Online Install

```sh
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash
```

`get.sh` downloads the latest release archive and `SHA256SUMS`, verifies the checksum, and runs `install.sh`.

### Offline Install from Release Archive

In the directory containing the downloaded tarball and `SHA256SUMS`:

```sh
sha256sum -c --ignore-missing SHA256SUMS
tar xzf frame-vrc-<version>.tar.gz && frame-vrc-<version>/install.sh
```

Files are installed to `~/.local/lib/frame-vrc` and symlinked to `~/.local/bin/frame-vrc` (open a new terminal if `frame-vrc` is not yet in `PATH`). The installer also compiles the VR dashboard panel locally (~1 minute).

## What `setup` Does

Running `frame-vrc setup` (or tapping **Setup to PCVR Version** in the panel) performs the following steps:

1. **Verifies Steam Dependencies**: Checks for FEX-Emu, Proton Experimental, and Proton EasyAntiCheat Runtime, opening Steam's install prompt for any missing components.
2. **Downloads & Verifies Runtime Packages**: Downloads the patched FEX build, DFR DXVK, H.264 video plugin, and Wine audio fix from GitHub Releases, verifying each package against its SHA-256 checksum.
3. **Configures Private FEX Environment**: Sets up ji10me's runtime inside a private FEX directory (`~/.local/share/frame-vrc/fex`) scoped only to VRChat, and compiles the session supervisor and eye-tracking bridge.
4. **Creates Standalone Proton Tool**: Creates `Proton Experimental (Ji10me PCVR Edit)`—a copy of Proton Experimental patched with DFR DXVK—at `~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr` (~1.5 GB). On first run, Steam restarts once to register the new compatibility tool.
5. **Configures VRChat in Steam**: Selects the custom Proton tool for VRChat, sets the default per-eye render resolution to `1920`, and configures launch options to `frame-vrc-run %command%`.

## CLI Reference

```text
frame-vrc setup                  Configure and switch VRChat to the PCVR build
frame-vrc official               Revert VRChat to the official Android build
frame-vrc status                 Show current version and component status
frame-vrc doctor                 Run environment health checks (prints "all set" when healthy)
frame-vrc resolution 1056|1440|1920  Set per-eye render resolution
frame-vrc launch                 Launch VRChat
frame-vrc update                 Update frame-vrc and runtime packages
frame-vrc uninstall [--purge]    Revert to the official build and uninstall frame-vrc
frame-vrc dfrtest [on|off]       Toggle DFR debug overlay (gaze indicator and DXVK DFR log)
frame-vrc video check            Verify whether H.264 hardware decoding is active during video playback
```

## Recommended VRChat Settings for Memory Optimization

To stay within the headset's 16 GB RAM budget:
- **Avatar Limits**: Hide Poor and Very Poor avatars, and lower limits for maximum shown avatars, download size, and uncompressed size.
- **Graphics**: Enable the Particle Limiter, keep mirror quality low or off, disable Anti-Aliasing, and set other graphics options to Low.

## Troubleshooting

If VRChat fails to launch:
1. Run `frame-vrc doctor` and follow any suggested fixes.
2. To capture a Proton log, temporarily prepend `PROTON_LOG=1` to `%command%` in VRChat's launch options (`PROTON_LOG=1 frame-vrc-run %command%`), reproduce the issue, and inspect `~/steam-438100.log` (remove `PROTON_LOG=1` afterward).
3. Include the output of `frame-vrc status` and `frame-vrc doctor` when reporting issues.

To review modified files or uninstall completely, see [System Changes & Uninstallation](changes.md).
