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

Running `frame-vrc setup` (or tapping **Setup to PCVR Version** in the panel) runs an 8-step guided wizard. If interrupted or stopped on an error, re-running `frame-vrc setup` (or tapping **Retry This Step**) resumes from the first unfinished step:

1. **Check Steam and VRChat**: Verifies Steam is responding and VRChat is in your library (prompts to add it if missing, or asks you to exit if running).
2. **Install FEX-Emu**: Checks FEX-Emu (`3127680`) and opens Steam's install prompt if missing, waiting for confirmation and completion.
3. **Install Proton Experimental**: Checks Proton Experimental (`1493710`), opening its Steam install prompt only after the previous tool is ready.
4. **Install Proton EasyAntiCheat Runtime**: Checks Proton EasyAntiCheat Runtime (`1826330`) using the same one-prompt-at-a-time flow.
5. **Get & Verify Packages**: Downloads (or uses custom local files for) the patched FEX build, DFR DXVK, H.264 video plugin, and Wine audio fix, verifying each SHA-256 checksum.
6. **Deploy the PCVR Runtime**: Compiles ji10me's session supervisor and eye-tracking bridge, deploys the private VRChat-only FEX environment (`~/.local/share/frame-vrc/fex`), and creates `Proton Experimental (Ji10me PCVR Edit)` at `~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr` (~1.5 GB).
7. **Let Steam See Proton**: Checks if Steam lists the new Proton copy; if not yet registered, restarts Steam once (the dashboard briefly disappears) and verifies again.
8. **Apply & Verify VRChat Settings**: Sets VRChat's compatibility tool, default per-eye render resolution (`1920`), and launch options (`frame-vrc-run %command%`), and notes if VRChat itself still needs an install or update in Steam.

## CLI Reference

```text
frame-vrc setup                  Configure and switch VRChat to the PCVR build (resumes if unfinished)
frame-vrc install-deps           Install only the Steam dependencies (steps 1–4)
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
