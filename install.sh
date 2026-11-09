#!/usr/bin/env bash
# frame-vrc installer: run ON THE STEAM FRAME as user steamos. Never uses sudo; writes only under $HOME.
# Copies the tool to ~/.local/lib/frame-vrc and links ~/.local/bin/frame-vrc; builds the VR panel (SteamVR dashboard)
# and adds it to the app list. Changes nothing in Steam by itself.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
[[ "$(id -u)" != 0 ]] || { echo "Do not run as root / with sudo. Run as your normal user (steamos)." >&2; exit 1; }
[[ "$(uname -m)" == aarch64 || "${FRAME_VRC_ALLOW_ANY_ARCH:-}" == 1 ]] ||
  { echo "This is for the Steam Frame (aarch64); this machine is $(uname -m)." >&2; exit 1; }
lib="$HOME/.local/lib/frame-vrc"
# Stage next to the target and swap, so an interrupted copy never leaves a half-updated tool.
rm -rf "$lib.new"
mkdir -p "$lib.new/lib" "$HOME/.local/bin"
cp "$here/frame-vrc" "$here/README.md" "$here/LICENSE" "$here/NOTICE" "$lib.new/"
cp -R "$here"/lib/. "$lib.new/lib/"
[[ ! -d $here/licenses ]] || cp -R "$here/licenses" "$lib.new/"
chmod +x "$lib.new/frame-vrc" "$lib.new"/lib/frame-vrc-run "$lib.new"/lib/dfr-run "$lib.new"/lib/*.sh "$lib.new"/lib/*.py
# The VR panel, built here (SteamOS ships cmake, ninja, g++ and the -dev files; it links SteamVR's own
# libopenvr_api.so). Without it the command line still does everything.
src=$here/panel-src; [[ -d $src ]] || src=$here/src/frame-vrc-panel
panel=false
if [[ -d $src ]] && command -v cmake >/dev/null && command -v ninja >/dev/null && command -v g++ >/dev/null; then
  echo "==> Building the VR panel (about a minute)"
  if cmake -G Ninja -S "$src" -B "$here/panel-build" -DCMAKE_BUILD_TYPE=Release >"$here/panel-build.log" 2>&1 &&
     ninja -C "$here/panel-build" >>"$here/panel-build.log" 2>&1; then
    strip -o "$lib.new/frame-vrc-panel" "$here/panel-build/frame-vrc-panel"
    "$lib.new/frame-vrc-panel" --icon-png 256 "$lib.new/frame-vrc.png" >/dev/null
    panel=true
  else
    # kept: get.sh deletes its temporary folder
    mkdir -p "$HOME/.local/share/frame-vrc" && cp "$here/panel-build.log" "$HOME/.local/share/frame-vrc/panel-build.log"
    tail -5 "$here/panel-build.log" >&2
    echo "the VR panel did not build (log: ~/.local/share/frame-vrc/panel-build.log); the command line works without it" >&2
  fi
else
  echo "no cmake/ninja/g++: the VR panel is skipped; the command line works without it" >&2
fi
# a panel opened from the app list would keep the old binary
pkill -TERM -x frame-vrc-panel 2>/dev/null || true
rm -rf "$lib" && mv "$lib.new" "$lib"
ln -sfn "$lib/frame-vrc" "$HOME/.local/bin/frame-vrc"
desktop=$HOME/.local/share/applications/frame-vrc.desktop
if $panel; then
  # absolute paths: Steam may start it without ~/.local/lib in PATH, and reads icons only when it starts
  mkdir -p "${desktop%/*}"
  sed -e "s|@BINARY@|$lib/frame-vrc-panel|" -e "s|@ICON@|$lib/frame-vrc.png|" "$src/contrib/frame-vrc.desktop" >"$desktop.tmp"
  mv -f "$desktop.tmp" "$desktop"
  echo "VR panel: SteamVR dashboard > + (Launch Program) > frame-vrc, or: frame-vrc panel"
else
  rm -f "$desktop"
fi
echo "installed frame-vrc $("$lib/frame-vrc" version) in $lib"
# An update: this release's packages (FEX, DFR, H.264) where they changed
if [[ -f $HOME/.local/share/frame-vrc/config || -d ${FRAME_VRC_WORK:-$HOME/fex-eac-work}/stage ]]; then
  echo "==> Checking the packages"
  "$lib/frame-vrc" packages || echo "the packages were not updated; frame-vrc doctor says what to do" >&2
fi
echo "next: ~/.local/bin/frame-vrc next"
