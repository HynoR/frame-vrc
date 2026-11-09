#!/usr/bin/env bash
# What frame-vrc leaves behind, on the headset: the files under $HOME (Steam's caches, logs and downloads and the
# games' own folders left out: they change by themselves) and the settings frame-vrc changes, before and after a round.
#   tools/residue-check.sh snap before.txt          (frame-vrc not installed, or after frame-vrc uninstall --purge)
#   ... install, setup, switch, update, frame-vrc uninstall --purge ...
#   tools/residue-check.sh snap after.txt
#   tools/residue-check.sh diff before.txt after.txt   "+" left behind, "-" gone; nothing printed = clean
# Read only; the lists hold file names and short hashes, no file contents. Keep them out of the repository.
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
steam=$HOME/.local/share/Steam apps=$HOME/.local/share/Steam/steamapps appid=438100

snap() {
  {
    find "$HOME" -xdev \( -path "$HOME/.cache" -o -path "$steam/logs" -o -path "$steam/appcache" -o -path "$steam/depotcache" \
      -o -path "$steam/config/htmlcache" -o -path "$steam/userdata" -o -path "$apps/common" -o -path "$apps/shadercache" \
      -o -path "$apps/downloading" -o -path "$apps/temp" -o -path "$apps/workshop" -o -path "$apps/compatdata/*/pfx" \
      -o -path "$HOME/.local/share/Steam/ubuntu12_32" -o -path "$HOME/.local/share/Steam/steamrtarm64" \) -prune -o -print 2>/dev/null |
      sed "s|^$HOME|~|; s|^|file |" || true  # folders we may not read (containers, overlays) make find fail
    # Steam's FEX-Emu (frame-vrc swaps its files) and VRChat's settings in Steam and SteamVR
    local f
    for f in "$apps/common/FEX-Emu"/usr/bin/* "$apps/common/FEX-Emu"/{FEXCompatTool,VERSIONS.txt,ConfigTemplate.json}; do
      [[ -f $f ]] && echo "fex ${f#"$apps/common/"} $(sha256sum "$f" | cut -c1-16)"
    done
    echo "setting compat-tool $(awk -v id="\"$appid\"" '/"CompatToolMapping"/{m=1} m&&$1==id{a=1} a&&$1=="\"name\""{print $2; exit}' \
      "$steam/config/config.vdf" 2>/dev/null)"
    local lo=$here/../lib/steam-launchopts.py
    echo "setting launch-options $({ python3 -I "$lo" get $appid 2>/dev/null || echo '(Steam did not answer)'; } | sha256sum | cut -c1-16)"
    echo "setting resolution $(timeout 10 /opt/steamvr/bin/linuxarm64/vrcmd --settings-int steam.app.$appid.resolutionOverride 2>/dev/null |
      sed -n 's/^.*=//p' | tail -1)"
  } | sort
}

case ${1:-} in
  snap) snap >"${2:?snap FILE}"; echo "$(grep -c '^file ' "$2") files listed in $2" ;;
  diff)
    comm -13 "${2:?diff BEFORE AFTER}" "${3:?diff BEFORE AFTER}" | sed 's/^/+ /'
    comm -23 "$2" "$3" | sed 's/^/- /' ;;
  *) sed -n '2,8p' "$0"; exit 1 ;;
esac
