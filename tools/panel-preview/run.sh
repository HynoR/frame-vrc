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
    # the stub where SteamVR has it (the panel'"'"'s RUNPATH), and an aborting one in LD_LIBRARY_PATH as Steam sets it:
    # the panel must drop LD_LIBRARY_PATH and use the first
    mkdir -p /opt/steamvr/bin/linuxarm64 /build/steamrt /tmp/h
    g++ -shared -fPIC -I/src/third_party/openvr -o /opt/steamvr/bin/linuxarm64/libopenvr_api.so /tools/openvr_stub.cpp
    g++ -shared -fPIC -DSTUB_ABORT -I/src/third_party/openvr -o /build/steamrt/libopenvr_api.so /tools/openvr_stub.cpp
    cmake -G Ninja -S /src -B /build/panel >/dev/null
    ninja -C /build/panel >/dev/null
    out=$1; shift
    # --show starts VR: with the aborting copy loaded it dies (134); dropping LD_LIBRARY_PATH it waits for SteamVR (124)
    rc=0; HOME=/tmp/s LD_LIBRARY_PATH=/build/steamrt timeout 5 /build/panel/frame-vrc-panel --show --cli /tools/fake-frame-vrc \
      >/dev/null 2>&1 || rc=$?
    [ $rc = 124 ] || { echo "panel exit $rc with Steam'"'"'s LD_LIBRARY_PATH (want 124: still running)" >&2; exit 1; }
    HOME=/tmp/h /build/panel/frame-vrc-panel --cli /tools/fake-frame-vrc --dump-png "/build/$out" "$@"
    ' sh "$@"
echo "$out/$1"
