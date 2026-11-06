#!/usr/bin/env bash
# Release tarball: dist/frame-vrc-<version>.tar.gz + SHA256SUMS. get.sh is not a release file: users fetch it from the
# repository (raw), and it downloads this tarball from the latest release.
set -euo pipefail
cd "$(dirname "$0")/.."
ver=$(sed -n 's/^VERSION=//p' frame-vrc)
name=frame-vrc-$ver
rm -rf dist && mkdir -p "dist/$name/lib"
cp frame-vrc install.sh README.md LICENSE NOTICE "dist/$name/"
cp lib/* "dist/$name/lib/"
# the VR panel's source: install.sh builds it on the headset
cp -R src/frame-vrc-panel "dist/$name/panel-src"
# COPYFILE_DISABLE: macOS tar would add ._* files; fixed owner: the archive must not carry the packager's user name
(cd dist && COPYFILE_DISABLE=1 tar $(tar --version | grep -q bsdtar && echo --no-xattrs --no-mac-metadata --uid 0 --gid 0 --uname root --gname root || echo --owner=0 --group=0 --numeric-owner) -czf "$name.tar.gz" "$name" &&
  { sha256sum "$name.tar.gz" 2>/dev/null || shasum -a 256 "$name.tar.gz"; } >SHA256SUMS && rm -rf "$name")
ls -l dist
