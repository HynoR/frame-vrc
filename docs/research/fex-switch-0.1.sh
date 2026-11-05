#!/bin/sh
# Switch Steam's FEX-Emu compat tool between Valve's build and the patched EAC build on the Steam Frame.
#   fex-switch.sh status     which build is installed: valve / patched / unknown
#   fex-switch.sh patched    install the patched build (reuses ~/fex-eac-work/stage, no rebuild)
#   fex-switch.sh valve      put Valve's build back and check it against the experiment baseline
# Wraps ~/fex-eac/install-fex-tool.sh; writes only under ~. Superseded by `frame-vrc switch`; kept for the record.
set -eu
V=$HOME/.local/share/Steam/steamapps/common/FEX-Emu
W=$HOME/fex-eac-work
T=$HOME/fex-eac/install-fex-tool.sh

state() {
  if (cd "$V" && sha256sum -c --quiet "$W/baseline/fex-sha256.txt" >/dev/null 2>&1); then echo valve; return; fi
  for b in FEX FEXBash FEXGetConfig FEXOfflineCompiler FEXServer FEXServerManager FEXpidof; do
    cmp -s "$W/stage/usr/bin/$b" "$V/usr/bin/$b" || { echo unknown; return; }
  done
  echo patched
}

# install-fex-tool.sh restore does not check this itself; swapping under a running FEXServer mixes builds.
idle() { if pgrep -x FEXServer >/dev/null; then echo "FEXServer running: quit the game first (or wait a few seconds)" >&2; exit 1; fi; }

# unknown = neither the baseline nor our stage, i.e. Steam updated FEX-Emu. Restoring the old backup would
# downgrade it, and installing over it would leave the new Valve build without a backup: stop and say so.
unknown() {
  echo "installed FEX is neither the baseline Valve build nor the patched stage (Steam update?):" >&2
  sed -n 's/^FEX describe\t/  installed: /p' "$V/VERSIONS.txt" >&2
  echo "Back up this build first, then install:" >&2
  echo "  BACKUP=$W/valve-backup-\$(date +%F) sh $T" >&2
  exit 1
}

s=$(state)
case "${1:-status}" in
status) echo "$s"; sh "$T" status;;
patched)
  case $s in patched) echo "already patched";; unknown) unknown;;
  valve) idle; sh "$T"; [ "$(state)" = patched ] || { echo "install did not take" >&2; exit 1; }; echo "now: patched";; esac;;
valve)
  case $s in valve) echo "already valve";; unknown) unknown;;
  patched) idle; sh "$T" restore; [ "$(state)" = valve ] || { echo "restore does not match the baseline" >&2; exit 1; }; echo "now: valve (matches baseline)";; esac;;
*) echo "usage: $0 status|patched|valve" >&2; exit 2;;
esac
