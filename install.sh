#!/usr/bin/env bash
# frame-vrc installer: run ON THE STEAM FRAME as user steamos. Never uses sudo; writes only under $HOME.
# Copies the tool to ~/.local/lib/frame-vrc and links ~/.local/bin/frame-vrc. Changes nothing in Steam by itself.
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
cp "$here"/lib/* "$lib.new/lib/"
chmod +x "$lib.new/frame-vrc" "$lib.new"/lib/*
rm -rf "$lib" && mv "$lib.new" "$lib"
ln -sfn "$lib/frame-vrc" "$HOME/.local/bin/frame-vrc"
echo "installed frame-vrc $("$lib/frame-vrc" version) in $lib"
echo "next: ~/.local/bin/frame-vrc doctor"
