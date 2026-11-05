# Install

**English** · [中文](install.zh-CN.md) · [日本語](install.ja.md)

Everything runs on the Frame, in Konsole or over SSH, as the normal user (`steamos`). Never use sudo.

## 1. Prerequisites (all methods)

1. In Steam, install **FEX-Emu** (Library > Tools), **Proton Experimental** and **Proton EasyAntiCheat Runtime**.
   With frame-vrc installed, `frame-vrc install-deps` opens Steam's install prompt for each missing one.
2. **VRChat > Properties > Compatibility > Proton Experimental.** Without this Steam installs the Android build. Let
   Steam download the Windows build.
3. Keep the headset on power for a first setup; SSH drops when it goes to sleep.

## 2. Install frame-vrc

Pick one:

```sh
# a) from the GitHub release (checks SHA256SUMS itself)
curl -fsSL https://github.com/HynoR/frame-vrc/releases/latest/download/get.sh | bash

# b) from a downloaded frame-vrc-<version>.tar.gz (e.g. from Discord): check it, then install
sha256sum -c --ignore-missing SHA256SUMS        # in the folder with the tarball and SHA256SUMS
tar xzf frame-vrc-<version>.tar.gz && sh frame-vrc-<version>/install.sh
```

It goes to `~/.local/lib/frame-vrc`, linked as `~/.local/bin/frame-vrc`. If `frame-vrc` is not found, open a new
shell or use `~/.local/bin/frame-vrc`.

## One command

After installing frame-vrc, `frame-vrc setup --resolution 720` does sections 1, 3 and 4 below by itself (prerequisites,
the latest release's FEX package, switch, launch options, resolution, doctor). Give it a URL or file to use another
package: `frame-vrc setup ~/Downloads/fex-<version>-steamframe-aarch64.tar.gz`. The sections below are the same steps
one at a time.

## 3. Get the patched FEX

### Method A: prebuilt package (recommended)

Download `fex-<version>-steamframe-aarch64.tar.gz` **and** its `.sha256` into the same folder, or give the release URL:

```sh
frame-vrc fetch ~/Downloads/fex-<version>-steamframe-aarch64.tar.gz      # a file from Discord
frame-vrc fetch https://github.com/.../fex-<version>-steamframe-aarch64.tar.gz   # a release asset
```

`fetch` refuses the package if the checksum does not match or the FEX in it does not run, and keeps the previous
build as `stage.prev`. The `-thunk-` package works the same way (optional, desktop only).

### Method B: build on the headset

```sh
frame-vrc build              # ~5 min with 4 jobs, ~2 GB in ~/fex-eac-work
```

The headset gets hot (up to ~98 C measured); the build stops itself at 100 C, and running it again continues.
Keep it on power and off your head.

### Method C: fully by hand (no frame-vrc)

1. Unpack a package: `mkdir -p ~/fex-eac-work && sha256sum -c fex-*.sha256 && tar xzf fex-*.tar.gz -C ~/fex-eac-work`
   (gives `~/fex-eac-work/stage`).
2. Quit every game (FEXServer must not run: `pgrep -x FEXServer` prints nothing).
3. Swap it in with steamac's script (also in this repo as `lib/install-fex-tool.sh`); it backs up Valve's files first:
   `sh install-fex-tool.sh` (`status` shows what is installed, `restore` puts Valve's files back).
4. Paste the launch options below into VRChat > Properties > Launch Options.
5. Optional resolution: `/opt/steamvr/bin/linuxarm64/vrcmd --settings-int steam.app.438100.resolutionOverride`
   (note the old value), then `--set-settings-int steam.app.438100.resolutionOverride 720`.

## 4. Switch and configure

```sh
frame-vrc switch eac             # backs up Valve's FEX, installs the patched one
frame-vrc launch-options set     # Steam sets them through its own API; `show` prints them to paste instead
frame-vrc resolution 720         # optional; 1056 is sharper; `reset` restores your old value
frame-vrc doctor
```

The launch options frame-vrc sets for the `eac` build:

```text
env DXVK_CONFIG="dxgi.maxDeviceMemory = 1024; dxvk.numCompilerThreads = 4" WINEDLLOVERRIDES="tabtip.exe=d" EAC_LAUNCHERDIR=$HOME/.local/share/Steam/steamapps/compatdata/438100/pfx/drive_c/users/steamuser/AppData/Roaming/EasyAntiCheat PROTON_EAC_RUNTIME="$HOME/.local/share/Steam/steamapps/common/Proton EasyAntiCheat Runtime" WINEDEBUG=-all PROTON_USE_XALIA=0 WINE_CPU_TOPOLOGY=16:0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7 %command%
```

What each part does: `EAC_LAUNCHERDIR` / `PROTON_EAC_RUNTIME` point EAC at Valve's runtime; `WINE_CPU_TOPOLOGY`
reports 16 CPUs (VRChat sizes its thread pool from that); `DXVK_CONFIG` reports 1 GB of VRAM and uses 4 shader compiler
threads; `tabtip.exe=d` drops Wine's touch keyboard (~135 MB); `PROTON_USE_XALIA=0` and `WINEDEBUG=-all` turn off
accessibility and logging work.

For the `thunk` build, `frame-vrc switch thunk` changes the line by itself (Vulkan thunk switch, driver fix,
`--no-vr`): see [thunk-build.md](thunk-build.md).

## 5. VRChat settings that save memory

In VRChat (Settings, in VR): hide Poor / Very Poor avatars, lower the avatar download and uncompressed size limits,
lower the maximum shown avatars, turn on the particle limiter, mirrors low or off. Other people's avatars fill memory
in busy worlds. Restarting the headset before playing gives Steam's UI processes a fresh start.

## 6. Check that it works

Start VRChat from Steam, in VR. The first world join takes about 3 minutes (FEX translates and caches the code),
later ones about 20 seconds. To record a session: run `frame-vrc bench` first, then start the game; the report lands in
`~/.local/share/frame-vrc/bench/` and says whether EAC started (`Launcher finished with: 301`,
`AntiCheat Session Begin: Success`).

If it does not start, add `PROTON_LOG=1` to the launch options once and look at `~/steam-438100.log`.

Undo everything: [changes.md](changes.md).
