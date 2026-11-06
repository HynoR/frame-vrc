# Install

**English** · [中文](install.zh-CN.md) · [日本語](install.ja.md)

Everything runs on the Frame, in Konsole or over SSH, as the normal user (`steamos`). Never use sudo.

## 1. Prerequisites

1. In Steam, install **Proton Experimental** and **Proton EasyAntiCheat Runtime** (and **FEX-Emu**, Library > Tools,
   if it is missing). With frame-vrc installed, `frame-vrc install-deps` opens Steam's install prompt for each.
2. **VRChat > Properties > Compatibility > Proton Experimental.** Without this Steam installs the Android build. Let
   Steam download the Windows build.
3. Start SteamVR once (frame-vrc sets the render resolution through it). Keep the headset on power for the first setup.

## 2. Install frame-vrc

```sh
# a) from GitHub: installs the latest release (checks SHA256SUMS itself)
curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash

# b) from a downloaded frame-vrc-<version>.tar.gz: check it, then install
sha256sum -c --ignore-missing SHA256SUMS        # in the folder with the tarball and SHA256SUMS
tar xzf frame-vrc-<version>.tar.gz && sh frame-vrc-<version>/install.sh
```

It goes to `~/.local/lib/frame-vrc`, linked as `~/.local/bin/frame-vrc` (open a new shell if `frame-vrc` is not found).

## 3. Setup

```sh
frame-vrc setup             # recommended: patched FEX, DFR Proton, 1920 + DFR, the tested options
frame-vrc setup --no-dfr    # without DFR: stock Proton Experimental
frame-vrc launch            # or start VRChat from Steam, in VR
```

`setup` stops and says what to do when something is missing in Steam; do that and run it again. It downloads two
packages from the latest release, each checked against its `.sha256`:

| Package | What | Instead of downloading |
|---|---|---|
| `fex-eac-<version>-steamframe-aarch64.tar.gz` | the patched FEX | `frame-vrc setup FILE` or `frame-vrc fetch FILE`; or build it on the headset: `frame-vrc build` (~5 min, gets hot, stops itself at 100 C) |
| `dfr-dxvk-<commit>-steamframe.tar.gz` | the DFR DXVK (`d3d11.dll`, `dxgi.dll`), the eye-tracking bridge and its OpenVR library | `frame-vrc setup --dfr FILE` or `frame-vrc proton dfr FILE`; or build it yourself (below) |

When a package comes from elsewhere, put its `.sha256` next to it.

The first `proton dfr` copies Proton Experimental to `~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr`
(~1.5 GB), swaps its DXVK and restarts Steam once so it lists the copy. When Steam updates Proton Experimental,
`doctor` says so; `frame-vrc proton dfr` copies it again.

### Building the DFR package yourself

Follow [ji10me/dxvk-steam-frame-dfr](https://github.com/ji10me/dxvk-steam-frame-dfr)'s guide (DXVK with MinGW-w64:
`meson setup build-frame --cross-file build-win64.txt --buildtype release -Denable_d3d8=false -Denable_d3d9=false
-Denable_d3d10=false`; the bridge on Linux ARM64 against OpenVR SDK `0924064`, linked with `-Wl,-rpath,'$ORIGIN'`),
then pack it with this repository's `tools/make-dfr-package.sh DXVK_SRC OPENVR_SDK`.

## 4. How DFR runs

With the DFR Proton, VRChat's launch options start with `…/lib/dfr-run`. It runs natively before the game: it sets
DXVK's DFR variables for the chosen level (`frame-vrc dfr`; `auto` = on from resolution 1600) and starts the
eye-tracking bridge, which begins once VRChat has the VR scene and ends with the game (log:
`~/.local/share/frame-vrc/bridge.log`). With the headset off your head the gaze is invalid and DXVK shades at full rate.

## 5. By hand (no frame-vrc)

1. Unpack the FEX package (`~/fex-eac-work/stage`), quit every game (`pgrep -x FEXServer` prints nothing) and run
   `sh lib/install-fex-tool.sh` (`restore` puts Valve's files back).
2. Paste the launch options (`frame-vrc launch-options show` prints them on a machine that has the tool), leaving out
   `…/dfr-run` unless you set up DFR as in ji10me's guide.
3. Resolution: `/opt/steamvr/bin/linuxarm64/vrcmd --set-settings-int steam.app.438100.resolutionOverride 1920`
   (note the old value first with `--settings-int steam.app.438100.resolutionOverride`).

## 6. VRChat settings that save memory

In VRChat (Settings, in VR): hide Poor / Very Poor avatars, lower the avatar download and uncompressed size limits,
lower the maximum shown avatars, turn on the particle limiter, mirrors low or off. Restarting the headset before
playing gives Steam's UI processes a fresh start.

## 7. Check that it works

`frame-vrc doctor` should end with `all set`. To record a session: run `frame-vrc bench` first, then start the game; the
report in `~/.local/share/frame-vrc/bench/` says whether EAC started (`Launcher finished with: 301`,
`AntiCheat Session Begin: Success`). If the game does not start, add `PROTON_LOG=1` to the launch options once and look
at `~/steam-438100.log`.

Undo everything: [changes.md](changes.md).
