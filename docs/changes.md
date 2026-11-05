# What changes and how to undo it

**English** · [中文](changes.zh-CN.md) · [日本語](changes.ja.md)

frame-vrc writes only under your home folder. This is everything it touches.

## Changed in Steam's files

| What | Where | Changed by | Undo |
|---|---|---|---|
| FEX-Emu: 7 programs (`FEX`, `FEXBash`, `FEXGetConfig`, `FEXOfflineCompiler`, `FEXServer`, `FEXServerManager`, `FEXpidof`) and `FEXCompatTool`, `VERSIONS.txt`, `ConfigTemplate.json` | `~/.local/share/Steam/steamapps/common/FEX-Emu/` | `switch eac` / `switch thunk` | `frame-vrc switch valve` (copies the backup back and compares every file) |
| FEX-Emu Vulkan thunks (`thunk` build only): `usr/lib/aarch64-linux-gnu/fex-emu/HostThunks/libvulkan-host.so`, `usr/share/fex-emu/GuestThunks/libvulkan-guest.so` | same folder | `switch thunk` | `switch eac` or `switch valve` (Valve's copies are in the backup) |

If the backup is ever lost: Steam > Library > Tools > FEX-Emu > uninstall, then install again; Steam brings back
Valve's files. When Steam updates FEX-Emu it overwrites the patched build: run `frame-vrc switch eac` again (the new
Valve build is backed up, the old backup kept as `valve-backup.old-<date>`).

## Changed in Steam / SteamVR settings

| What | Where | Changed by | Undo |
|---|---|---|---|
| VRChat launch options | Steam (saved in `~/.local/share/Steam/userdata/*/config/localconfig.vdf`) | `launch-options set`, `switch` (only if frame-vrc set them) | `frame-vrc launch-options clear` (puts back what you had, kept in `~/.local/share/frame-vrc/launch-options.438100.prev`), or delete them in VRChat > Properties |
| VRChat render resolution | SteamVR (`~/.config/openvr/config/steamvr.vrsettings`, `steam.app.438100` > `resolutionOverride`) | `resolution N` (through SteamVR's `vrcmd`) | `frame-vrc resolution reset` (old value kept in `~/.local/share/frame-vrc/resolution.438100.prev`), or SteamVR > Settings > Video > per-application |

## Created by frame-vrc

| What | Where | Removed by |
|---|---|---|
| The tool | `~/.local/lib/frame-vrc`, `~/.local/bin/frame-vrc` | `frame-vrc uninstall` |
| **Backup of Valve's FEX** (keep it) | `~/fex-eac-work/valve-backup/FEX-<version>/` (+ `thunks/`) | `uninstall --purge`, only after Valve's build is back |
| Builds and sources | `~/fex-eac-work/` (`stage`, `stage-mod`, `*.prev`, `FEX`, `build-*`, `build.log`) | `uninstall --purge` |
| Thunk driver list | `~/.local/share/frame-vrc/vk/freedreno_host.aarch64.json` | `uninstall --purge` |
| Benchmark reports, packages | `~/.local/share/frame-vrc/bench/`, `~/.local/share/frame-vrc/dist/` | `uninstall --purge`, or delete |

## Created while playing (by Steam, FEX and Proton, not frame-vrc)

| What | Where | Remove |
|---|---|---|
| FEX code cache (~0.8 GB after a few sessions, one file set per FEX setting; makes later starts fast) | `~/.local/share/Steam/steamapps/shadercache/438100/fex-emu/` | delete the folder; it is rebuilt |
| VRChat's Wine prefix (EAC files, game settings, caches) | `~/.local/share/Steam/steamapps/compatdata/438100/` | uninstalling VRChat in Steam |

## Not touched

The system (`/usr`, `/etc`, `/opt`), firmware, `/persist`, other partitions, Proton, the EAC Runtime, VRChat's game
files, other games. Native and Proton ARM64 games never use FEX.

## Full uninstall

Quit the game first (FEX must not be running), with SteamVR running for the resolution reset:

```sh
frame-vrc uninstall           # Valve's FEX back, launch options cleared, resolution reset, tool removed;
                              # builds, backup and reports kept
frame-vrc uninstall --purge   # also deletes ~/fex-eac-work and ~/.local/share/frame-vrc
```

Proton Experimental, the EAC Runtime and VRChat stay in Steam; set VRChat's compatibility back to default to get the
Android build again.

### By hand (installed with method C)

```sh
sh install-fex-tool.sh restore                       # Valve's FEX back from ~/fex-eac-work/valve-backup
/opt/steamvr/bin/linuxarm64/vrcmd --set-settings-int steam.app.438100.resolutionOverride <old value>
rm -rf ~/fex-eac-work                                # after checking `sh install-fex-tool.sh status`
```

and delete the launch options in VRChat > Properties.
