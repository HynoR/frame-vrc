#!/usr/bin/env python3
"""Turn frame-vrc's private FEX copy into ji10me's tested VRChat runtime (SteamFrame_Standalone_PCVR_VRChat 0.3.6,
installer/frame_setup.py prepare_runtime; MIT, Copyright (c) 2026 JITOME, see lib/ji10me/LICENSE).

  fex-runtime.py FEX_DIR CONFIG_DIR VIDEO_DIR GAZE_FILE ADAPTER_C FINAL_DIR

FEX_DIR     the private copy (Steam's FEX-Emu + the patched binaries), changed in place
CONFIG_DIR  where the FEX app configs and the Vulkan ICD file go (frame-vrc-run copies ThunksDB.json to the prefix)
VIDEO_DIR   a folder holding only the H.264 GStreamer plugin ("" = none)
GAZE_FILE   the gaze file the bridge writes (a Linux path)
ADAPTER_C   lib/ji10me/vulkan_lookup_adapter.c
FINAL_DIR   where FEX_DIR is moved afterwards: the adapter finds Valve's thunk there (its RUNPATH)

Vulkan thunks only with the thunk files ji10me tested (Valve's FEX-Emu 2607-76). Seen once, they are kept in
CONFIG_DIR/tested-thunks: after Steam updates FEX-Emu the copy still gets the tested ones. Else thunks stay off.
Prints "thunks on" or "thunks off: REASON".
"""
import hashlib, json, os, shutil, subprocess, sys

fex, cfg, video, gaze, adapter, final = sys.argv[1:7]
SYSROOT = "/usr/share/guestos/fex-mesa"
HOST = "usr/lib/aarch64-linux-gnu/fex-emu/HostThunks/libvulkan-host.so"
GUEST_DIR = "usr/share/fex-emu/GuestThunks"
DB = "usr/share/fex-emu/ThunksDB.json"
# ji10me 0.3.6 compatibility.json: Valve's FEX-Emu FEX-2607-76-g37265b1
TESTED = {HOST: "127f9c354be74861", GUEST_DIR + "/libvulkan-guest.so": "46333327f3ca54df", DB: "a0be4a09185abf61"}


def sha(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def thunks():
    keep = os.path.join(cfg, "tested-thunks")
    for rel, want in TESTED.items():
        p, k = os.path.join(fex, rel), os.path.join(keep, os.path.basename(rel))
        if os.path.isfile(p) and sha(p).startswith(want):
            if not (os.path.isfile(k) and sha(k).startswith(want)):
                os.makedirs(keep, exist_ok=True)
                shutil.copy2(p, k)
        elif os.path.isfile(k) and sha(k).startswith(want):
            shutil.copy2(k, p)  # Steam's FEX-Emu moved on: the tested file, as kept
        else:
            return f"{rel} is not the tested one (Steam's FEX-Emu is not 2607-76 and none was kept)"
    for tool in ("clang", "ld.lld", "patchelf"):
        if not shutil.which(tool):
            return f"{tool} missing"
    g = os.path.join(fex, GUEST_DIR)
    base = os.path.join(g, "libframe_vulkan_thunk_base.so")
    # the adapter answers FEX's Vulkan self-lookups (FEX upstream #5940) and hands everything else to Valve's thunk
    shutil.copy2(os.path.join(g, "libvulkan-guest.so"), base)
    subprocess.check_call(["patchelf", "--set-soname", "libframe_vulkan_thunk_base.so", base])
    subprocess.check_call(["clang", "--target=x86_64-linux-gnu", f"--sysroot={SYSROOT}", "-fuse-ld=lld", "-shared", "-fPIC",
                           "-nostdlib", adapter, f"-L{g}", "-Wl,--no-as-needed", "-l:libframe_vulkan_thunk_base.so",
                           f"-L{SYSROOT}/usr/lib", "-lc", "-Wl,-z,defs", f"-Wl,-rpath,{os.path.join(final, GUEST_DIR)}", "-Wl,-soname,libvulkan.so.1",
                           "-o", os.path.join(g, "libvulkan-guest.so.new")])
    os.replace(os.path.join(g, "libvulkan-guest.so.new"), os.path.join(g, "libvulkan-guest.so"))
    # the guest's Vulkan loader under every name the container may load it by
    with open(os.path.join(fex, DB)) as f:
        db = json.load(f)
    overlay = db["DB"]["Vulkan"].setdefault("Overlay", [])
    real = os.path.basename(os.path.realpath(f"{SYSROOT}/usr/lib/libvulkan.so.1"))
    for d in (f"{SYSROOT}/usr/lib", "/usr/lib", "/run/gfx/main/usr/lib"):
        for n in ("libvulkan.so.1", real):
            if f"{d}/{n}" not in overlay:
                overlay.append(f"{d}/{n}")
    for path in (os.path.join(fex, DB), os.path.join(cfg, "ThunksDB.json")):
        with open(path, "w") as f:
            json.dump(db, f, indent=2)
    with open(os.path.join(cfg, "arm64-icd.json"), "w") as f:
        json.dump({"file_format_version": "1.0.1", "ICD": {
            "library_path": "/usr/lib/pressure-vessel/overrides/lib/aarch64-linux-gnu/vulkan/libvulkan_freedreno.so",
            "library_arch": "64", "api_version": "1.4.362"}}, f)
    return ""


os.makedirs(cfg, exist_ok=True)
off = thunks()
# FEX: no code cache, no profiling (ji10me: DiskCache 0, ProfileStats 0)
tpl = os.path.join(fex, "ConfigTemplate.json")
with open(tpl) as f:
    t = json.load(f)
t.setdefault("Config", {}).update({"DiskCache": "0", "ProfileStats": "0"})
with open(tpl, "w") as f:
    json.dump(t, f, indent=2)
# Setup processes without thunks; the game's container with them. FEX passes "Env" to the emulated processes (repeated
# keys, as FEX reads them): the H.264 plugin and its Iris settings, which the launch line alone may not get across.
with open(os.path.join(cfg, "setup-app.json"), "w") as f:
    json.dump({"Config": {}, "ThunksDB": {"Vulkan": 0}}, f)
env = []
if video:
    env = [f"GST_PLUGIN_PATH_1_0={video}", "GST_PLUGIN_FEATURE_RANK=v4l2h264dec:512,v4l2vp9dec:0,v4l2h265dec:0",
           "FRAME_V4L2_IRIS_RESTART=1", "FRAME_V4L2_IRIS_POOL=1"]
with open(os.path.join(cfg, "video-app.json"), "w") as f:
    f.write('{"Config":{' + ",".join('"Env":' + json.dumps(e) for e in env) + '},"ThunksDB":{"Vulkan":%d}}' % (0 if off else 1))
# Steam's container: ji10me's DXVK, DFR (always on: 70% of each eye at full rate, the rest 2x2, HDR passes only) and
# Vulkan driver choice
emu = os.path.join(fex, "emulator.json")
with open(emu) as f:
    e = json.load(f)
v = e["emulator_v0"]
v["main_argv"] = "./usr/bin/FEX"
v.setdefault("environment", {})["FEX_APP_CONFIG"] = os.path.join(cfg, "setup-app.json")
ce = v.setdefault("container_environment", {})
ce.update({"FEX_APP_CONFIG": os.path.join(cfg, "video-app.json"),
           "DXVK_CONFIG": "dxvk.trackPipelineLifetime = True; dxvk.numCompilerThreads = 2; dxgi.maxDeviceMemory = 2048",
           "PROTON_USE_XALIA": "0",
           "DXVK_GAZE_VRS": "1", "DXVK_GAZE_FMT": "97", "DXVK_GAZE_COARSE": "5", "DXVK_GAZE_FULL_AREA": "70",
           "DXVK_GAZE_MID_AREA": "85", "DXVK_GAZE_DEBUG": "0", "DXVK_GAZE_DOT": "0", "DXVK_GAZE_FLIP_Y": "0",
           "DXVK_GAZE_FILE": "Z:" + gaze.replace("/", "\\")})
if not off:
    ce.update({"VK_DRIVER_FILES": os.path.join(cfg, "arm64-icd.json"), "VK_INSTANCE_LAYERS": "", "FDM_DEBUG": "",
               "RPO_DEBUG": ""})
with open(emu, "w") as f:
    json.dump(e, f, indent=2)
print("thunks on" if not off else f"thunks off: {off}")
