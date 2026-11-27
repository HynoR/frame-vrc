#!/usr/bin/env bash
# Lightweight CLI simulation for PCVR setup wizard (steps 1-4, status JSON, lock, reopen, official clear).
# Runs the real frame-vrc in a temp HOME with a fake Steam library and stubbed steam-launchopts.py.
set -euo pipefail

repo=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
bg_pid=""
cleanup() {
  [[ -n $bg_pid ]] && kill "$bg_pid" 2>/dev/null || true
  rm -rf "$tmp"
}
trap cleanup EXIT

mkdir -p "$tmp/app/lib" "$tmp/home/.local/share/Steam/steamapps/common" "$tmp/home/.local/share/Steam/userdata/1/config"
mkdir -p "$tmp/sd/steamapps/common"
cp "$repo/frame-vrc" "$tmp/app/frame-vrc"
cp -R "$repo/lib/." "$tmp/app/lib/"
rm -rf "$tmp/app/lib/__pycache__"
chmod +x "$tmp/app/frame-vrc"

url_log=$tmp/steam_urls.log
: >"$url_log"
cat >"$tmp/app/lib/steam-launchopts.py" <<'PY'
import os, sys

op = sys.argv[1] if len(sys.argv) > 1 else ""
if op == "ping":
    sys.exit(0)
if op == "owned":
    print("1")
    sys.exit(0)
if op == "get":
    print("")
    sys.exit(0)
if op in ("set", "compat", "restart", "run", "stop"):
    sys.exit(0)
if op == "url":
    with open(os.environ["STEAM_URL_LOG"], "a", encoding="utf-8") as f:
        f.write(sys.argv[2] + "\n")
    sys.exit(0)
sys.exit(1)
PY

export HOME=$tmp/home
export STEAM_URL_LOG=$url_log
steamapps=$HOME/.local/share/Steam/steamapps
sdapps=$tmp/sd/steamapps

cat >"$steamapps/libraryfolders.vdf" <<EOF
"libraryfolders"
{
  "0"
  {
    "path" "$HOME/.local/share/Steam"
  }
  "1"
  {
    "path" "$tmp/sd"
  }
}
EOF

# Fake VRChat installed + pre-installed EAC Runtime (1826330)
cat >"$steamapps/appmanifest_438100.acf" <<'EOF'
"AppState"
{
  "appid" "438100"
  "StateFlags" "4"
}
EOF
cat >"$steamapps/appmanifest_1826330.acf" <<'EOF'
"AppState"
{
  "appid" "1826330"
  "StateFlags" "4"
}
EOF
mkdir -p "$steamapps/common/Proton EasyAntiCheat Runtime/v2/lib64"
touch "$steamapps/common/Proton EasyAntiCheat Runtime/v2/lib64/libeasyanticheat.so"

cli=$tmp/app/frame-vrc
fail() { echo "FAIL: $*" >&2; exit 1; }
assert_json_field() {
  local json=$1 expr=$2
  python3 -I -c "import json,sys; d=json.loads(sys.argv[1]); assert ($expr), f'failed: $expr in {d}'" "$json" ||
    fail "JSON assertion ($expr) on: $json"
}
wait_for_status() {
  local expr=$1 i s
  for i in $(seq 1 30); do
    s=$("$cli" setup-status --json)
    if python3 -I -c "import json,sys; d=json.loads(sys.argv[1]); sys.exit(0 if ($expr) else 1)" "$s" 2>/dev/null; then
      printf '%s\n' "$s"
      return 0
    fi
    sleep 0.5
  done
  fail "timed out waiting for status ($expr); last: ${s:-none}"
}
count_url() {
  grep -cxF "$1" "$url_log" 2>/dev/null || true
}

echo "1. setup-status with no progress file"
s=$("$cli" setup-status --json)
[[ $s == '{"active":false}' ]] || fail "expected {\"active\":false}, got: $s"
printf '%s\n' "$s" | python3 -m json.tool >/dev/null

echo "2. Start setup: FEX missing -> only 3127680 requested, lock blocks duplicate start"
FRAME_VRC_DEP_TIMEOUT=30 "$cli" setup >"$tmp/setup1.log" 2>&1 &
bg_pid=$!

s=$(wait_for_status 'd.get("state") == "waiting_user" and d.get("step") == 2 and d.get("app_id") == "3127680"')
printf '%s\n' "$s" | python3 -m json.tool >/dev/null
assert_json_field "$s" 'd["active"] is True and d["op"] == "setup" and d["total"] == 8'
assert_json_field "$s" 'd["download"] is False and d["alive"] is True and "reopen" in d["actions"]'
assert_json_field "$s" 'len(d["results"]) == 8 and d["results"][0]["state"] == "done" and d["results"][1]["state"] == "waiting"'
[[ $(count_url "steam://install/3127680") -eq 1 ]] || fail "expected 1 FEX request, got $(cat "$url_log")"

if "$cli" setup >"$tmp/dup.log" 2>&1; then
  fail "duplicate setup should fail while setup.lock is held"
fi
sleep 1.5
[[ $(count_url "steam://install/3127680") -eq 1 ]] || fail "polling/duplicate start must not re-send 3127680"

echo "3. Explicit setup-reopen re-sends 3127680 once"
"$cli" setup-reopen >/dev/null
for _ in $(seq 1 20); do
  [[ $(count_url "steam://install/3127680") -eq 2 ]] && break
  sleep 0.25
done
[[ $(count_url "steam://install/3127680") -eq 2 ]] || fail "setup-reopen did not re-send 3127680"

echo "4. Download activity sets download:true and waiting_steam"
mkdir -p "$steamapps/downloading/3127680"
s=$(wait_for_status 'd.get("state") == "waiting_steam" and d.get("download") is True')
assert_json_field "$s" 'd["app_id"] == "3127680"'
rmdir "$steamapps/downloading/3127680"

echo "5. Mark FEX ready -> advances to Proton Experimental (1493710) once"
mkdir -p "$steamapps/common/FEX-Emu"
touch "$steamapps/common/FEX-Emu/fex-compat-tool" "$steamapps/common/FEX-Emu/VERSIONS.txt" "$steamapps/common/FEX-Emu/emulator.json"
cat >"$steamapps/appmanifest_3127680.acf" <<'EOF'
"AppState"
{
  "appid" "3127680"
  "StateFlags" "4"
}
EOF

s=$(wait_for_status 'd.get("state") == "waiting_user" and d.get("step") == 3 and d.get("app_id") == "1493710"')
assert_json_field "$s" 'd["results"][1]["state"] == "done" and d["results"][2]["state"] == "waiting"'
[[ $(count_url "steam://install/1493710") -eq 1 ]] || fail "expected 1 Proton request"

echo "6. Timeout & resume must not re-send 1493710"
kill "$bg_pid" 2>/dev/null || true
wait "$bg_pid" 2>/dev/null || true
bg_pid=""

s=$("$cli" setup-status --json)
assert_json_field "$s" 'd["state"] == "interrupted" and d["alive"] is False and "retry" in d["actions"]'

set +e
FRAME_VRC_DEP_TIMEOUT=1 "$cli" setup >"$tmp/setup2.log" 2>&1
rc=$?
set -e
[[ $rc -ne 0 ]] || fail "timed-out setup should exit non-zero"
[[ $(count_url "steam://install/1493710") -eq 1 ]] || fail "resumed run must not re-send 1493710"
s=$("$cli" setup-status --json)
assert_json_field "$s" 'd["state"] == "interrupted" and d["step"] == 3 and d["app_id"] == "1493710"'

echo "7. Mark Proton ready in SD library + verify EAC (1826330) gets zero requests via install-deps"
mkdir -p "$sdapps/common/Proton - Experimental/files/lib/wine/dxvk/x86_64-windows" \
         "$sdapps/common/Proton - Experimental/files/lib/wine/x86_64-windows"
touch "$sdapps/common/Proton - Experimental/proton" "$sdapps/common/Proton - Experimental/version" \
      "$sdapps/common/Proton - Experimental/files/lib/wine/dxvk/x86_64-windows/d3d11.dll" \
      "$sdapps/common/Proton - Experimental/files/lib/wine/dxvk/x86_64-windows/dxgi.dll" \
      "$sdapps/common/Proton - Experimental/files/lib/wine/x86_64-windows/mf.dll"
cat >"$sdapps/appmanifest_1493710.acf" <<'EOF'
"AppState"
{
  "appid" "1493710"
  "StateFlags" "4"
}
EOF

FRAME_VRC_DEP_TIMEOUT=5 "$cli" install-deps >"$tmp/deps.log" 2>&1
s=$("$cli" setup-status --json)
printf '%s\n' "$s" | python3 -m json.tool >/dev/null
assert_json_field "$s" 'd["op"] == "install-deps" and d["state"] == "done" and d["step"] == 4 and d["total"] == 4 and d["alive"] is False'
[[ $(count_url "steam://install/1826330") -eq 0 ]] || fail "pre-installed EAC must receive zero requests"

echo "8. official clears progress and intent"
"$cli" official >"$tmp/official.log" 2>&1
s=$("$cli" setup-status --json)
[[ $s == '{"active":false}' ]] || fail "expected official to clear setup-progress.json, got: $s"
[[ ! -e $HOME/.local/share/frame-vrc/setup-intent ]] || fail "expected official to clear setup-intent"

echo "9. A stale progress file naming a reused pid: not alive, and official does not kill that process"
sleep 60 &
other=$!
printf '{"active":true,"pid":%d,"op":"setup","state":"waiting_user","step":2,"total":8,"app_id":"3127680","results":[]}' \
  "$other" >"$HOME/.local/share/frame-vrc/setup-progress.json"
s=$("$cli" setup-status --json)
assert_json_field "$s" 'd["alive"] is False and d["state"] == "interrupted" and "retry" in d["actions"]'
"$cli" official >"$tmp/official2.log" 2>&1
kill -0 "$other" 2>/dev/null || fail "official killed an unrelated process with a reused pid"
kill "$other" 2>/dev/null || true

echo "10. A finished setup waits on VRChat only while it is not the PC build yet"
printf '{"active":true,"pid":1,"op":"setup","state":"done","step":8,"total":8,"game_pending":true,"results":[]}' \
  >"$HOME/.local/share/frame-vrc/setup-progress.json"
s=$("$cli" setup-status --json)
assert_json_field "$s" 'd["game_pending"] is True'
mkdir -p "$steamapps/common/VRChat"; touch "$steamapps/common/VRChat/VRChat.exe"
s=$("$cli" setup-status --json)
assert_json_field "$s" 'd["game_pending"] is False'

echo "setup-sim.sh: ALL PASSED"
