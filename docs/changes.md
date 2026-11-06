# What changes and how to undo it

**English** · [中文](changes.zh-CN.md) · [日本語](changes.ja.md)

frame-vrc writes only under your home folder. This is everything it touches.

## Changed in Steam's files

| What | Where | Changed by | Undo |
|---|---|---|---|
| FEX-Emu: 7 programs (`FEX`, `FEXBash`, `FEXGetConfig`, `FEXOfflineCompiler`, `FEXServer`, `FEXServerManager`, `FEXpidof`) and `FEXCompatTool`, `VERSIONS.txt`, `ConfigTemplate.json` | `~/.local/share/Steam/steamapps/common/FEX-Emu/` | `fex eac` | `frame-vrc fex valve` (copies the backup back and compares every file) |

If the backup is ever lost: Steam > Library > Tools > FEX-Emu > uninstall, then install again; Steam brings back
Valve's files. When Steam updates FEX-Emu it overwrites the patched build: run `frame-vrc fex eac` again (the new
Valve build is backed up, the old backup kept as `valve-backup.old-<date>`).

Proton Experimental is never modified: DFR uses a copy (below).

## Changed in Steam / SteamVR settings

| What | Where | Changed by | Undo |
|---|---|---|---|
| VRChat launch options | Steam (saved in `~/.local/share/Steam/userdata/*/config/localconfig.vdf`) | `launch-options set`, and every switch (only if frame-vrc set them) | `frame-vrc launch-options clear` (puts back what you had, kept in `~/.local/share/frame-vrc/launch-options.438100.prev`), or delete them in VRChat > Properties |
| VRChat compatibility tool | Steam (`~/.local/share/Steam/config/config.vdf`, `CompatToolMapping`) | `proton dfr` / `proton stock` | `frame-vrc proton stock`, or VRChat > Properties > Compatibility > Proton Experimental |
| VRChat render resolution | SteamVR (`~/.config/openvr/config/steamvr.vrsettings`, `steam.app.438100` > `resolutionOverride`) | `resolution N` (through SteamVR's `vrcmd`) | `frame-vrc resolution reset` (old value kept in `~/.local/share/frame-vrc/resolution.438100.prev`), or SteamVR > Settings > Video > per-application |

## Created by frame-vrc

| What | Where | Removed by |
|---|---|---|
| The tool | `~/.local/lib/frame-vrc`, `~/.local/bin/frame-vrc` | `frame-vrc uninstall` |
| **Backup of Valve's FEX** (keep it) | `~/fex-eac-work/valve-backup/FEX-<version>/` | `uninstall --purge`, only after Valve's build is back |
| The patched FEX build (and sources if built here) | `~/fex-eac-work/` (`stage`, `stage.prev`, `FEX`, `build-*`, `build.log`) | `uninstall --purge` |
| The DFR Proton (a copy of Proton Experimental with the DFR DXVK; originals in `dxvk-orig/`) | `~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr/` (~1.5 GB) | `frame-vrc uninstall`, or delete the folder after `proton stock` |
| Settings, DFR package, bridge log, benchmark reports | `~/.local/share/frame-vrc/` (`config`, `dfr/`, `bridge.log`, `bench/`, `dist/`) | `uninstall --purge`, or delete |
| The bridge's gaze file | `~/.local/share/frame-pcvr/gaze/frame-gaze-v1.bin` (64 bytes, emptied when the bridge stops) | delete the folder |

## Created while playing (by Steam, FEX and Proton, not frame-vrc)

| What | Where | Remove |
|---|---|---|
| FEX code cache (~1 GB after a few sessions; makes later starts fast) | `~/.local/share/Steam/steamapps/shadercache/438100/fex-emu/` | delete the folder; it is rebuilt |
| VRChat's Wine prefix (EAC files, game settings, caches) | `~/.local/share/Steam/steamapps/compatdata/438100/` | uninstalling VRChat in Steam |

## Not touched

The system (`/usr`, `/etc`, `/opt`), firmware, `/persist`, other partitions, Proton Experimental itself, the EAC Runtime,
VRChat's game files, other games. Native and Proton ARM64 games never use FEX.

## Full uninstall

Quit the game first (FEX must not be running), with SteamVR running for the resolution reset:

```sh
frame-vrc uninstall           # Valve's FEX back, Proton Experimental for VRChat, DFR copy deleted, launch options
                              # cleared, resolution reset, tool removed; FEX build, backup, settings and reports kept
frame-vrc uninstall --purge   # also deletes ~/fex-eac-work and ~/.local/share/frame-vrc
```

Proton Experimental, the EAC Runtime and VRChat stay in Steam; set VRChat's compatibility back to default to get the
Android build again.

### By hand

```sh
sh install-fex-tool.sh restore                       # Valve's FEX back from ~/fex-eac-work/valve-backup
/opt/steamvr/bin/linuxarm64/vrcmd --set-settings-int steam.app.438100.resolutionOverride <old value>
rm -rf ~/fex-eac-work ~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr   # after checking `install-fex-tool.sh status`
```

then set VRChat > Properties > Compatibility > Proton Experimental and delete its launch options.
