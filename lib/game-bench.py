#!/usr/bin/env python3
# Read-only sampler for one Steam game session: every 5 s, CPU and RSS of the game's process tree (everything under
# Steam's reaper for the app), system memory, swap (zram) and the hottest thermal zone; in VR, once a minute,
# SteamVR's frame stats (vrcmd --stats). Prints a Markdown table row set when the game exits.
#   game-bench.py APPID LOG [WAIT_S]    waits up to WAIT_S (default 600) for the game to start; ends 60 s after it exits
import glob, json, os, subprocess, sys, time

APP, LOG = sys.argv[1], sys.argv[2]
WAIT = int(sys.argv[3]) if len(sys.argv) > 3 else 600
FOLLOW = 60
HZ = os.sysconf("SC_CLK_TCK")
PAGE_MB = os.sysconf("SC_PAGE_SIZE") / 2**20


def rd(p):
    try:
        with open(p) as f:
            return f.read()
    except OSError:
        return ""


def procs():
    # pid -> (ppid, cpu ticks, rss pages); comm may contain spaces, so split after the last ')'
    out = {}
    for d in glob.glob("/proc/[0-9]*"):
        s = rd(d + "/stat")
        if s:
            f = s[s.rfind(")") + 2:].split()
            out[int(d[6:])] = (int(f[1]), int(f[11]) + int(f[12]), int(f[21]))
    return out


def reaper():
    for d in glob.glob("/proc/[0-9]*"):
        c = rd(d + "/cmdline").split("\0")
        if c and c[0].endswith("/reaper") and f"AppId={APP}" in c:
            return int(d[6:])


def tree(ps, root):
    kids, out, todo = {}, set(), [root]
    for pid, (pp, _, _) in ps.items():
        kids.setdefault(pp, []).append(pid)
    while todo:
        p = todo.pop()
        out.add(p)
        todo += kids.get(p, [])
    return out


def pss_mb(pid):
    # proportional set size: shared pages split between their users, so the tree's sum is what it really costs
    for l in rd(f"/proc/{pid}/smaps_rollup").splitlines():
        if l.startswith("Pss:"):
            return int(l.split()[1]) / 1024
    return 0


UNIT = {"": 1 / 2**20, "B": 1 / 2**20, "KiB": 1 / 1024, "MiB": 1, "GiB": 1024}


def gpu_mb(pids):
    # drm-total-memory per DRM client (msm fdinfo); one client can be open through several fds, count it once
    seen, total = set(), 0
    for p in pids:
        for fd in glob.glob(f"/proc/{p}/fdinfo/*"):
            info = rd(fd)
            if "drm-driver:" not in info:
                continue
            kv = dict(l.split(":", 1) for l in info.splitlines() if ":" in l)
            cid = kv.get("drm-client-id", "").strip()
            if cid in seen:
                continue
            seen.add(cid)
            v = kv.get("drm-total-memory", "0").split()
            total += int(v[0]) * UNIT.get(v[1] if len(v) > 1 else "", 0)
    return total


def cpu_total():
    f = [int(x) for x in rd("/proc/stat").split("\n")[0].split()[1:]]
    return sum(f), f[3] + f[4]


def meminfo():
    return {l.split(":")[0]: int(l.split()[1]) // 1024 for l in rd("/proc/meminfo").splitlines()}


VRCMD = "/opt/steamvr/bin/linuxarm64/vrcmd"


def vrstats():
    # SteamVR's compositor stats for this app (cumulative since it started); None outside VR. IPC only, the game's
    # memory is not touched.
    try:
        out = subprocess.run([VRCMD, "--stats"], capture_output=True, text=True, timeout=8).stdout
        s = next(x for x in json.loads(out[out.index("["):]) if x.get("key") == f"steam.app.{APP}")
        return time.time(), s
    except (OSError, ValueError, StopIteration, subprocess.TimeoutExpired):
        return None


def vr_summary(first, last):
    # Frames the game submitted per second between two readings, and how many shown frames SteamVR had to reproject.
    (t0, a), (t1, b) = first, last
    fps = (b["frame_submits"] - a["frame_submits"]) / (t1 - t0)
    rep = (b["reprojected_frames"] - a["reprojected_frames"]) / max(b["frame_presents"] - a["frame_presents"], 1)
    return (f"| SteamVR (after the first minute) | game {fps:.1f} fps, {100 * rep:.0f}% of shown frames reprojected; "
            f"per frame CPU {b['average_application_cpu_time_ms']:.1f} ms, GPU {b['average_application_gpu_time_ms']:.1f} ms |")


def main():
    end = time.time() + WAIT
    root = reaper()
    while not root and time.time() < end:
        time.sleep(2)
        root = reaper()
    if not root:
        sys.exit(f"no game with AppId={APP} started")
    rows = []
    vr_first = vr_last = None
    prev_ps, prev_t, prev_sys = procs(), time.time(), cpu_total()
    with open(LOG, "a", buffering=1) as log:
        log.write(f"# start {time.strftime('%F %T')} app={APP} cols: time sys_cpu% game_cpu%(100=one core) "
                  "game_rss_mb mem_avail_mb swap_used_mb temp_c game_pss_mb game_gpu_mb\n")
        while True:
            if not os.path.exists(f"/proc/{root}"):
                # A launch starts with a short-lived process (~10 s) before the real one: follow the next launch
                # of the same app if it comes within FOLLOW seconds.
                end = time.time() + FOLLOW
                root = None
                while not root and time.time() < end:
                    time.sleep(2)
                    root = reaper()
                if not root:
                    break
                log.write(f"# follow {time.strftime('%T')} next launch\n")
            time.sleep(5)
            ps, t, sysc = procs(), time.time(), cpu_total()
            pids = [p for p in tree(ps, root) if p in ps]
            # processes new since the last sample count from 0
            ticks = sum(ps[p][1] - (prev_ps[p][1] if p in prev_ps else 0) for p in pids)
            dt, di = sysc[0] - prev_sys[0], sysc[1] - prev_sys[1]
            m = meminfo()
            temp = max((int(rd(z) or 0) for z in glob.glob("/sys/class/thermal/thermal_zone*/temp")), default=0)
            r = (100 * (dt - di) / max(dt, 1), 100 * ticks / HZ / (t - prev_t), sum(ps[p][2] for p in pids) * PAGE_MB,
                 m["MemAvailable"], m["SwapTotal"] - m["SwapFree"], temp / 1000,
                 sum(pss_mb(p) for p in pids), gpu_mb(pids))
            if len(pids) > 1:  # the reaper alone means the game is shutting down
                rows.append(r)
                if len(rows) % 12 == 1:  # once a minute
                    v = vrstats()
                    if v:
                        vr_first = vr_first or v
                        vr_last = v
            log.write(time.strftime("%T ") + " ".join(f"{x:.0f}" for x in r[:5]) + f" {r[5]:.1f} {r[6]:.0f} {r[7]:.0f}\n")
            prev_ps, prev_t, prev_sys = ps, t, sysc
        log.write(f"# end {time.strftime('%F %T')}\n")
    if not rows:
        sys.exit("the game exited before the first sample")
    col = lambda i: [r[i] for r in rows]
    avg = lambda i: sum(col(i)) / len(rows)
    psi = rd("/proc/pressure/memory").split("\n")
    full300 = next((w.split("=")[1] for l in psi if l.startswith("full") for w in l.split() if w.startswith("avg300")), "?")
    print(f"| session | {len(rows) * 5 / 60:.1f} min, {len(rows)} samples |")
    print(f"| game CPU (100% = one core, {os.cpu_count()} cores) | avg {avg(1):.0f}%, max {max(col(1)):.0f}% |")
    print(f"| system CPU | avg {avg(0):.0f}% |")
    print(f"| game RSS / PSS | max {max(col(2)):.0f} / {max(col(6)):.0f} MB |")
    print(f"| game GPU memory (drm) | max {max(col(7)):.0f} MB |")
    print(f"| available memory | avg {avg(3):.0f} MB, min {min(col(3)):.0f} MB, "
          f"{sum(x < 1024 for x in col(3))} samples under 1 GB |")
    print(f"| swap (zram) used | max {max(col(4)):.0f} MB of {meminfo()['SwapTotal']} MB |")
    print(f"| memory pressure (PSI full avg300 at exit) | {full300}% |")
    print(f"| temperature | avg {avg(5):.1f} C, max {max(col(5)):.1f} C |")
    if vr_last and vr_last is not vr_first:
        print(vr_summary(vr_first, vr_last))


main()
