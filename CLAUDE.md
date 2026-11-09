# frame-vrc: notes for agents

frame-vrc lets the Steam Frame (SteamOS VR, aarch64, 16 GB) run the PC build of VRChat with EAC. It is an
**installer and switcher**: it installs ji10me's runtime
([SteamFrame_Standalone_PCVR_VRChat](https://github.com/ji10me/SteamFrame_Standalone_PCVR_VRChat)) from the headset
alone, switches VRChat between the official Android version (Lepton) and the PCVR version, and puts everything back.

- **The runtime is ji10me's and is authoritative.** He tested it with the community; do not tune it (no DFR levels,
  CPU/VRAM/thread knobs, extra env vars). Our work is isolation, install, switching and the panel.
- Users install only ji10me's versions. Syncing a new ji10me release is done by hand (his release format is not fixed
  yet); see "Syncing ji10me" below. Third-party builds are installed by the user with `setup --dfr/--video/--mf FILE`.

## Hard rules

- Never sudo. Write only under `$HOME`. Never touch `/usr`, `/etc`, `/opt`, `/persist`, other partitions, or
  thermal/battery/frequency sysfs.
- Every change must be reversible by `frame-vrc official` / `uninstall`.
- Never modify Steam's FEX-Emu, Proton Experimental, the EAC runtime or game files. Never move Steam game files and
  never start Steam downloads (Steam updates VRChat itself at launch).
- Never read a running VRChat's full `/proc/<pid>/smaps` (it makes the game stutter); use `smaps_rollup`.
- No binaries in git (release assets only; `dist/`, `build/`, `tmp/` are ignored). No `__pycache__` in the repo.
- No personal data, headset IPs, chat history or Valve-internal URLs in the repo. User docs are for ordinary users:
  short, no test narratives.
- Program text (CLI, panel, comments) is English; user docs exist in English, 中文 and 日本語 and change together.
- Commit, push, tag and publish releases only with the maintainer's explicit OK.

## How it works

```
Steam: <launch options> = frame-vrc-run %command%
  frame-vrc-run (lib/frame-vrc-run), only when the command uses our Proton copy:
    swaps Steam's FEX-Emu/fex-compat-tool for ~/.local/share/frame-vrc/fex/fex-compat-tool (private copy)
    FEX_APP_CONFIG_LOCATION / FEX_APP_DATA_LOCATION in VRChat's prefix, own FEXServer socket, EAC env,
    STEAM_COMPAT_TOOL_PATHS rewritten, starts ji10me's session.py (thread affinity + gaze bridge), then exec
  -> private FEX (ji10me's emulator.json env: DXVK, DFR, Vulkan thunks, H.264 via FEX "Env")
  -> compatibilitytools.d/frame-vrc-dfr = "Proton Experimental (Ji10me PCVR Edit)"
     (copy of Proton Experimental + ji10me's DFR d3d11/dxgi + his mf.dll when Proton's mf.dll hash matches)
```

Version switch = VRChat's compatibility tool only: `lepton` (official) or `frame-vrc-dfr` (PCVR). Any other tool:
`frame-vrc-run` runs the command unchanged.

## Layout

| Path | What |
|---|---|
| `frame-vrc` | The CLI (bash, one file). `VERSION=` at the top. Commands dispatched at the bottom. |
| `lib/frame-vrc-run` | The launch wrapper above. `lib/dfr-run` is a shim for old launch options. |
| `lib/fex-runtime.py` | Applies ji10me's `prepare_runtime` to the private FEX copy (emulator.json, ConfigTemplate, app configs, thunk adapter + ThunksDB overlays). Keeps his tested thunk files in `fex-config/tested-thunks` so a Valve FEX update does not turn thunks off. |
| `lib/ji10me/` | His sources, unmodified (MIT): session.py, gaze bridge, openvr-stats, Vulkan lookup adapter. Built on the headset by `runtime_install`. |
| `lib/steam-launchopts.py` | Reads/sets launch options through Steam's CEF (Decky's approach). |
| `install.sh`, `get.sh` | Installer (copies `lib/` recursively, builds the panel); one-line installer fetched from `main`. |
| `src/frame-vrc-panel/` | VR dashboard panel (C++, cairo, OpenVR). Runs the CLI; reads `status --json`, `doctor --json`. |
| `tools/` | `make-package.sh` (release tarball), `flow-check.sh` (headset checks), `residue-check.sh`, `panel-preview/` (render the panel to PNG in Docker). `make-dfr-package.sh` is from 0.7 (built DFR from source) and is not used for 0.8. |
| `docs/` | User docs, three languages; `docs/images/` screenshots (English UI only). |
| `NOTICE`, `licenses/` | Third-party notices; FEX's licenses. |

On the headset: `~/.local/lib/frame-vrc` (tool), `~/.local/share/frame-vrc/` (`config`, `fex/`, `fex-config/`,
`runtime/`, `dfr/`, `video/`, `mf/`, `cache/`, `*.prev` backups), `~/fex-eac-work/stage` (FEX package),
`~/.local/share/Steam/compatibilitytools.d/frame-vrc-dfr`. Full list: `docs/changes.md`.

## Conventions in `frame-vrc`

- `*_state` functions print `ok | old | missing`; `*_install` / `dfr_copy` stage into `X.new`, then swap. Anything
  that bakes a path must use the **final** path, not `X.new` (the thunk adapter's RUNPATH once pointed at `fex.new`
  and the game crashed).
- `die` for errors; `quit_first` before changing anything VRChat or FEX uses; `running` checks the game.
- Packages: `pkg_fetch TOP DEST FILES SRC` expects a tarball with one top folder (`dfr/`, `video/`, `mf/`) holding
  the files and a one-line `VERSION`, plus `NAME.sha256` next to it. Release assets are found by name pattern
  (`fex-eac-*`, `dfr-dxvk-*`, `video-h264-*`, `wine-mf-*`); a package given on the command line is pinned.
- Steam restarts only when Steam has not yet listed the Proton copy (`steam_lists_copy`, from `compat_log.txt`).
- Comments explain why, in plain sentences; match the surrounding density. No new dependencies (bash, python3 stdlib,
  tools SteamOS ships: clang, clang++, ld.lld, patchelf).

## Testing

- `bash -n frame-vrc`, then the sandbox flow test if you have one (fake Steam API + fake FEX; macOS needs Homebrew
  bash). Remove `lib/__pycache__` after any `py_compile`.
- Panel: `tools/panel-preview/run.sh out.png --lang en [--do press:view:install ...]`, scenarios via
  `FAKE=ready|fresh|official|nogame|own|download|update`. Fixtures in `tools/panel-preview/`.
- On the headset (as `steamos`, over SSH): install from a `make-package.sh` tarball, `frame-vrc setup`,
  `frame-vrc doctor` (ends `all set`), `tools/flow-check.sh community|official|clean`. With the game running,
  flow-check also checks the private FEX, the FEXServer socket and the thread layout. `frame-vrc dfrtest on` shows
  eye dots and writes DXVK's GazeVRS log; turn it off afterwards.
- Logs: `PROTON_LOG=1` in front of `%command%` once → `~/steam-438100.log`; `~/.local/share/frame-vrc/runtime/session.log`;
  VRChat's `output_log_*.txt` in the prefix (`AntiCheat Session Begin: Success`).

## Syncing ji10me

When he releases: compare his `compatibility.json` (tested Steam FEX/Proton hashes → `TESTED` in `fex-runtime.py`,
`mf_tested` in `frame-vrc`), `prebuilt_fex.py` (FEX commit; our FEX package must be the same commit),
`frame_setup.py` `prepare_runtime`/`prepare_prefix` (env, configs → `fex-runtime.py`), `launch.py` (→ `frame-vrc-run`),
`session.py` and `support/` (→ `lib/ji10me/`), and his payload (`dfr/`, `video/`, `wine/` → our `dfr-dxvk-*`,
`video-h264-*`, `wine-mf-*` packages with licenses and the matching source assets). Bump `ji_version`. Take his
runtime as is; keep our isolation (private FEX copy, Steam's prefix, Proton copy, Lepton switch).

## Release

1. `VERSION=` in `frame-vrc`, the version in `tools/panel-preview/status-*.json`, `docs/history*.md`.
2. `tools/make-package.sh` → `dist/frame-vrc-<ver>.tar.gz`.
3. Assets: the tarball, every package with its `.sha256`, the source archives (GStreamer, DXVK, Wine mf), and a
   `SHA256SUMS` covering all of them. A stable release must be marked Latest (`get.sh` installs
   `/releases/latest`).
4. Only after the maintainer's OK: commit, tag `v<ver>`, push, `gh release create`.
