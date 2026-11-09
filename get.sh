#!/usr/bin/env bash
# One-line install or update of frame-vrc on the Steam Frame (Konsole or SSH, as user steamos; never sudo):
#   curl -fsSL https://raw.githubusercontent.com/HynoR/frame-vrc/main/get.sh | bash
#   ... | FRAME_VRC_VERSION=0.1.0 bash   a given release instead of the latest (a pre-release too: 0.8.0-rc2)
# Fetched from the repository (raw) so fixes here reach users at once; it installs only released versions: downloads
# the latest (or given) release tarball + SHA256SUMS, checks the sum, and runs install.sh from a temporary folder.
set -euo pipefail

# everything sits in main, so a download cut off half way runs nothing
main() {
  local repo=https://github.com/HynoR/frame-vrc ver="${FRAME_VRC_VERSION:-}" tmp name
  if [[ -z "$ver" ]]; then
    # /releases/latest redirects to /releases/tag/vX.Y.Z (no API token, no jq needed)
    ver=$(curl -fsSLI -o /dev/null -w '%{url_effective}' "$repo/releases/latest")
    ver=${ver##*/v}
  fi
  [[ "$ver" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-rc[0-9]+)?$ ]] || { echo "could not find the latest frame-vrc release" >&2; exit 1; }
  name="frame-vrc-$ver"
  tmp=$(mktemp -d)
  # Expanded now: the trap runs after main's locals are gone (set -u would fail on $tmp).
  trap "rm -rf '$tmp'" EXIT
  echo "==> Downloading frame-vrc $ver"
  curl -fL --progress-bar -o "$tmp/$name.tar.gz" "$repo/releases/download/v$ver/$name.tar.gz"
  curl -fsSL -o "$tmp/SHA256SUMS" "$repo/releases/download/v$ver/SHA256SUMS"
  (cd "$tmp" && sha256sum -c --ignore-missing SHA256SUMS) || { echo "checksum mismatch: not installing" >&2; exit 1; }
  tar xzf "$tmp/$name.tar.gz" -C "$tmp"
  "$tmp/$name/install.sh"
}

main "$@"
