#!/usr/bin/env python3
# Read or set a game's Steam launch options.
#   steam-launchopts.py get APPID    print them, as Steam saved them in localconfig.vdf
#   steam-launchopts.py set APPID    set them to stdin, through Steam's own API
#   steam-launchopts.py stop APPID   quit the game, as Steam's Stop button does
#   steam-launchopts.py compat APPID TOOL   set the game's compatibility tool
#   steam-launchopts.py restart | ping      restart Steam / check that it answers
# Setting goes through Steam's CEF debug port (127.0.0.1:8080, SharedJSContext), the way Decky does: Steam keeps
# localconfig.vdf in memory and rewrites it, so editing the file would be lost. Reading the file is reliable once
# Steam saved it; appDetailsStore can return a stale empty value.
import base64, glob, json, os, re, socket, struct, sys, urllib.request

PORT = 8080


def ws_path():
    for t in json.load(urllib.request.urlopen(f"http://127.0.0.1:{PORT}/json", timeout=5)):
        if t.get("title") == "SharedJSContext":
            return t["webSocketDebuggerUrl"].split(str(PORT), 1)[1]
    sys.exit("Steam's SharedJSContext not found (is Steam running?)")


def evaluate(expr):
    s = socket.create_connection(("127.0.0.1", PORT), timeout=10)
    key = base64.b64encode(os.urandom(16)).decode()
    s.sendall(f"GET {ws_path()} HTTP/1.1\r\nHost: 127.0.0.1:{PORT}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
              f"Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n".encode())
    f = s.makefile("rb")
    if b" 101 " not in f.readline():
        sys.exit("websocket handshake with Steam failed")
    while f.readline() not in (b"\r\n", b""):
        pass
    msg = json.dumps({"id": 1, "method": "Runtime.evaluate",
                      "params": {"expression": expr, "awaitPromise": True, "returnByValue": True}}).encode()
    n, mask = len(msg), os.urandom(4)
    # Client frames must be masked (RFC 6455).
    hdr = b"\x81" + (bytes([0x80 | n]) if n < 126 else bytes([0x80 | 126]) + struct.pack(">H", n))
    s.sendall(hdr + mask + bytes(b ^ mask[i % 4] for i, b in enumerate(msg)))
    buf = b""
    while True:
        b0, b1 = f.read(2)
        n = b1 & 0x7F
        if n == 126:
            n = struct.unpack(">H", f.read(2))[0]
        elif n == 127:
            n = struct.unpack(">Q", f.read(8))[0]
        buf += f.read(n)
        if not b0 & 0x80:
            continue
        r = json.loads(buf)
        buf = b""
        if r.get("id") == 1:
            s.close()
            if "exceptionDetails" in r.get("result", {}):
                sys.exit(f"Steam API call failed: {r['result']['exceptionDetails']}")
            return r


def get(app):
    # The first "<app>" block that holds LaunchOptions (Software > Valve > Steam > apps); VDF escapes \ and ".
    for path in glob.glob(os.path.expanduser("~/.local/share/Steam/userdata/*/config/localconfig.vdf")):
        s = open(path, encoding="utf-8", errors="replace").read()
        for m in re.finditer(r'"%s"\s*\{((?:[^{}]|\{[^{}]*\})*)\}' % re.escape(app), s):
            v = re.search(r'"LaunchOptions"\s*"((?:[^"\\]|\\.)*)"', m.group(1))
            if v:
                return re.sub(r"\\(.)", r"\1", v.group(1))
    return ""


op = sys.argv[1]
if op == "restart":  # Steam lists compatibility tools only at start
    evaluate("SteamClient.User.StartRestart(false)")
    sys.exit()
if op == "ping":  # Steam's UI is up and answering
    evaluate("1")
    sys.exit()
app = sys.argv[2]
if not app.isdigit():
    sys.exit("APPID must be a number")
if op == "compat":  # the game's compatibility tool, as Properties > Compatibility sets it
    evaluate(f"SteamClient.Apps.SpecifyCompatTool({app}, {json.dumps(sys.argv[3])})")
elif op == "get":
    print(get(app))
elif op == "set":
    evaluate(f"SteamClient.Apps.SetAppLaunchOptions({app}, {json.dumps(sys.stdin.read())})")
elif op == "run":  # as the Play button does
    evaluate(f"SteamClient.Apps.RunGame('{app}', '', -1, 100)")
elif op == "stop":
    evaluate(f"SteamClient.Apps.TerminateApp('{app}', false)")
else:
    sys.exit("usage: steam-launchopts.py get|set|run|stop APPID | compat APPID TOOL | restart | ping")
