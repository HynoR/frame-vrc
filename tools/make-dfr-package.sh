#!/usr/bin/env bash
# DFR package for frame-vrc proton dfr: dfr-dxvk-<commit>-steamframe.tar.gz (+ .sha256), top folder dfr/.
#   tools/make-dfr-package.sh DXVK_SRC OPENVR_SDK [OUT_DIR]
# DXVK_SRC: a checkout of ji10me/dxvk-steam-frame-dfr (branch steam-frame-dfr) built as its README says, so it holds
# build-frame/src/{d3d11/d3d11.dll,dxgi/dxgi.dll} and frame/bin/frame-gaze-bridge (linked with -Wl,-rpath,'$ORIGIN'
# so it finds libopenvr_api.so next to it). OPENVR_SDK: Valve's openvr checkout the bridge was built against.
set -euo pipefail
src=${1:?DXVK_SRC} sdk=${2:?OPENVR_SDK} out=${3:-dist}
commit=$(git -C "$src" rev-parse --short=8 HEAD)
name=dfr-dxvk-$commit-steamframe
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
d=$tmp/dfr; mkdir -p "$d/licenses"
cp "$src/build-frame/src/d3d11/d3d11.dll" "$src/build-frame/src/dxgi/dxgi.dll" "$src/frame/bin/frame-gaze-bridge" "$d/"
cp "$sdk/lib/linuxarm64/libopenvr_api.so" "$d/"
# Unstripped they are ~3x larger; same code.
if command -v x86_64-w64-mingw32-strip >/dev/null; then x86_64-w64-mingw32-strip "$d/d3d11.dll" "$d/dxgi.dll"; fi
# DXVK and the Frame additions: zlib; the GAZ2 header: MIT; OpenVR: BSD-3-Clause.
cp "$src/LICENSE" "$d/licenses/DXVK.txt"
cp "$src/frame/LICENSE" "$d/licenses/frame-additions.txt"
cp "$src/frame/licenses/DFR-UPSTREAM.txt" "$d/licenses/DFR-UPSTREAM.txt"
cp "$sdk/LICENSE" "$d/licenses/OpenVR.txt"
echo "dfr-dxvk $commit (github.com/ji10me/dxvk-steam-frame-dfr), OpenVR SDK $(git -C "$sdk" rev-parse --short=8 HEAD)" >"$d/VERSION"
mkdir -p "$out"
# COPYFILE_DISABLE: macOS tar would add ._* files; fixed owner: the archive must not carry the packager's user name
(cd "$tmp" && COPYFILE_DISABLE=1 tar $(tar --version | grep -q bsdtar && echo --no-xattrs --no-mac-metadata --uid 0 --gid 0 --uname root --gname root || echo --owner=0 --group=0 --numeric-owner) -czf "$name.tar.gz" dfr)
mv "$tmp/$name.tar.gz" "$out/"
(cd "$out" && { sha256sum "$name.tar.gz" 2>/dev/null || shasum -a 256 "$name.tar.gz"; } >"$name.tar.gz.sha256")
ls -l "$out/$name.tar.gz" "$out/$name.tar.gz.sha256"
