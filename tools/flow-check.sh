#!/usr/bin/env bash
# Checks, on the headset, what frame-vrc promises after each step (read only):
#   tools/flow-check.sh community   after frame-vrc setup: our Proton copy + private FEX + launch options, Steam's FEX-Emu Valve's
#   tools/flow-check.sh official    after frame-vrc official: Lepton, no launch options of ours, resolution back
#   tools/flow-check.sh clean       after frame-vrc uninstall --purge: nothing of frame-vrc left, VRChat on Lepton
# Prints one line per check and "N failed"; exit 1 when any failed.
set -uo pipefail
here=$(cd "$(dirname "$0")" && pwd)
steam=$HOME/.local/share/Steam apps=$steam/steamapps appid=438100
fex=$apps/common/FEX-Emu data=$HOME/.local/share/frame-vrc copy=$steam/compatibilitytools.d/frame-vrc-dfr
fails=0
check() {  # TEXT COMMAND...
  local t=$1; shift
  if "$@" >/dev/null 2>&1; then echo "ok    $t"; else echo "FAIL  $t"; fails=$((fails + 1)); fi
}
tool() {
  awk -v id="\"$appid\"" '/"CompatToolMapping"/{m=1} m&&$1==id{a=1} a&&$1=="\"name\""{gsub(/"/,"",$2); print $2; exit}' \
    "$steam/config/config.vdf" 2>/dev/null
}
opts() { python3 -I "$here/../lib/steam-launchopts.py" get $appid 2>/dev/null || python3 -I "$HOME/.local/lib/frame-vrc/lib/steam-launchopts.py" get $appid; }
res() { timeout 10 /opt/steamvr/bin/linuxarm64/vrcmd --settings-int steam.app.$appid.resolutionOverride 2>/dev/null | sed -n 's/^.*=//p' | tail -1; }
valve_fex() { [[ $(sed -n 's/^FEX describe\t//p' "$fex/VERSIONS.txt") != eac-* ]]; }
eq() { [[ $1 == "$2" ]]; }
registered() {  # Steam's last scan of compatibilitytools.d listed the copy
  awk '/Processing local tool list/ { if (!scan) seen = 0; scan = 1; next }
       scan && /Registering tool / { if ($5 == "frame-vrc-dfr,") seen = 1; next }
       { scan = 0 } END { exit !seen }' "$steam/logs/compat_log.txt"
}

check "Steam's FEX-Emu is Valve's build" valve_fex
case ${1:-} in
  community)
    check "VRChat's compatibility tool: frame-vrc-dfr" eq "$(tool)" frame-vrc-dfr
    check "Steam has the Proton copy listed" registered
    check "copy named Proton Experimental (Ji10me PCVR Edit)" grep -qF '"Proton Experimental (Ji10me PCVR Edit)"' "$copy/compatibilitytool.vdf"
    check "copy has the DFR DXVK" cmp -s "$copy/files/lib/wine/dxvk/x86_64-windows/d3d11.dll" "$data/dfr/d3d11.dll"
    check "private FEX: patched build" grep -q 'eac-' "$data/fex/VERSIONS.txt"
    check "private FEX: Valve's compat tool script (less its environment dump)" bash -c "diff -q <(grep -v printenv '$data/fex/fex-compat-tool') <(grep -v printenv '$fex/fex-compat-tool')"
    check "launch options: frame-vrc-run %command%" eq "$(opts)" "\$HOME/.local/lib/frame-vrc/lib/frame-vrc-run %command%"
    check "H.264 package" test -f "$data/video/libgstvideo4linux2.so"
    check "ji10me runtime in the private FEX" grep -q '^ji10me-' "$data/fex/.frame-vrc-runtime"
    check "Vulkan thunks on" grep -q 'thunks on' "$data/fex/.frame-vrc-runtime"
    check "thunk adapter finds Valve's thunk (RUNPATH)" bash -c "readelf -d '$data/fex/usr/share/fex-emu/GuestThunks/libvulkan-guest.so' | grep -q 'runpath: \[$data/fex/usr/share/fex-emu/GuestThunks\]'"
    check "session supervisor and bridge built" test -x "$data/runtime/gaze-bridge" -a -x "$data/runtime/openvr-stats"
    check "resolution is frame-vrc's" eq "$(res)" "$(sed -n 's/^RES=//p' "$data/config" 2>/dev/null | tail -1 | grep . || echo 1920)"
    if pgrep -x VRChat.exe >/dev/null; then
      p=$(pgrep -x VRChat.exe | head -1)
      check "game runs on the private FEX" grep -q "$data/fex/" "/proc/$p/maps"
      check "game has the EAC variables" grep -qz '^EAC_LAUNCHERDIR=' "/proc/$p/environ"
      check "FEXServer from the private FEX" bash -c "pgrep -af FEXServer | grep -q '$data/fex/'"
      check "game has its own FEXServer socket" grep -qz '^FEX_SERVERSOCKETPATH=frame-vrc-vrchat' "/proc/$p/environ"
      check "session supervisor: thread layout applied" grep -q '"tested_thread_affinity_applied": true' "$data/runtime/session.log"
    fi ;;
  official)
    check "VRChat's compatibility tool: lepton" eq "$(tool)" lepton
    check "no launch options of frame-vrc's" bash -c "! grep -qE 'frame-vrc-run|EAC_LAUNCHERDIR' <<<\"\$1\"" _ "$(opts)"
    check "resolution not changed by frame-vrc (no saved value)" test ! -f "$data/resolution.$appid.prev" ;;
  clean)
    check "VRChat's compatibility tool: lepton" eq "$(tool)" lepton
    check "launch options empty" eq "$(opts)" ""
    check "no Proton copy" test ! -e "$copy"
    check "no frame-vrc data" test ! -e "$data"
    check "no frame-vrc program" test ! -e "$HOME/.local/lib/frame-vrc"
    check "no frame-vrc command" test ! -e "$HOME/.local/bin/frame-vrc"
    check "no FEX work folder" test ! -e "$HOME/fex-eac-work"
    check "no gaze file" test ! -e "$HOME/.local/share/frame-pcvr/gaze/frame-gaze-v1.bin" ;;
  *) sed -n '2,6p' "$0"; exit 2 ;;
esac
echo "$fails failed"
((fails == 0))
