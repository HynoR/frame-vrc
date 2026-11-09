# System Changes & Uninstallation

**English** · [中文](changes.zh-CN.md) · [日本語](changes.ja.md)

`frame-vrc` writes exclusively within your user home directory (`~`) and never uses `sudo`. Below is a complete list of modified settings and created paths.

## Steam & SteamVR Settings (VRChat Only)

| Setting | Modified By | Reverted By |
|---|---|---|
| VRChat compatibility tool (`Proton Experimental (Ji10me PCVR Edit)`) | `setup` | `official` (reverts to Default / `Lepton`) |
| VRChat launch options (`frame-vrc-run %command%`) | `setup` | `official` (restores backup from `~/.local/share/frame-vrc/launch-options.438100.prev`) |
| VRChat per-eye render resolution in SteamVR | `setup`, `resolution N` | `official` (restores backup from `~/.local/share/frame-vrc/resolution.438100.prev`) |

## Files & Directories Created

| Component | Path |
|---|---|
| CLI tool and VR dashboard panel | `~/.local/lib/frame-vrc`, `~/.local/bin/frame-vrc`, `~/.local/share/applications/frame-vrc.desktop` |
| Standalone Proton copy (~1.5 GB) | `~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr/` |
| Private FEX runtime, downloaded packages, and state files | `~/.local/share/frame-vrc/` |
| Downloaded FEX archive cache | `~/fex-eac-work/` |
| Eye-tracking bridge gaze state (64 bytes) | `~/.local/share/frame-pcvr/gaze/` |
| FEX thunk configuration for VRChat | `~/.local/share/Steam/steamapps/compatdata/438100/fex-emu/ThunksDB.json` |

## Unmodified System Components

The following remain completely untouched:
- System directories (`/usr`, `/etc`, `/opt`), firmware, and other partitions
- Steam's built-in FEX-Emu, upstream Proton Experimental, and Proton EasyAntiCheat Runtime
- VRChat's game installation files and all other Steam games

## Uninstallation

Quit VRChat before uninstalling, and keep SteamVR running so the render resolution can be restored automatically.

```sh
# Revert VRChat to the official build and remove the Proton copy, private FEX, and frame-vrc (restarts Steam once)
frame-vrc uninstall

# Complete removal: also delete ~/.local/share/frame-vrc and ~/fex-eac-work caches
frame-vrc uninstall --purge
```

Note: Proton Experimental, Proton EasyAntiCheat Runtime, and VRChat remain installed in Steam. VRChat's Wine prefix (`steamapps/compatdata/438100`) is managed by Steam and removed when VRChat is uninstalled via Steam.

### Manual Recovery

To revert everything manually without the CLI:
1. In Steam, open **VRChat > Properties > Compatibility** and select **Default (Lepton)**.
2. In **VRChat > Properties > General**, clear the launch options.
3. Delete the directories listed under **Files & Directories Created** above.
