#!/usr/bin/env bash
# Render the VR panel to a PNG on macOS/Linux with Docker (arm64 host or emulation), no SteamVR and no headset:
#   tools/panel-preview/run.sh home.png
#   FAKE=fresh tools/panel-preview/run.sh env.png --do press:view:env
# Builds the panel against a stub libopenvr_api and drives it against fake-frame-vrc (scenario FAKE=ready|fresh).
# Writes build/preview/OUT.png (build/ is not in git).
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
tools="$root/tools/panel-preview" out="$root/build/preview"
mkdir -p "$out"
docker build -q --platform linux/arm64 -t frame-vrc-panel-dev "$tools" >/dev/null
docker run --rm --platform linux/arm64 -e FAKE="${FAKE:-ready}" -e FAKE_SLOW="${FAKE_SLOW:-1}" \
  -v "$root/src/frame-vrc-panel":/src:ro -v "$tools":/tools:ro -v "$out":/build \
  frame-vrc-panel-dev sh -c '
    set -e
    mkdir -p /build/ovr /tmp/h
    g++ -shared -fPIC -I/src/third_party/openvr -o /build/ovr/libopenvr_api.so /tools/openvr_stub.cpp
    cmake -G Ninja -S /src -B /build/panel -DOPENVR_LIB_DIR=/build/ovr >/dev/null
    ninja -C /build/panel >/dev/null
    out=$1; shift
    HOME=/tmp/h LD_LIBRARY_PATH=/build/ovr /build/panel/frame-vrc-panel --cli /tools/fake-frame-vrc --dump-png "/build/$out" "$@"
    ' sh "$@"
echo "$out/$1"
