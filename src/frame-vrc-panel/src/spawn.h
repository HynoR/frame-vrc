// Running the frame-vrc CLI from the panel: short queries (output captured) and actions started detached, so they
// outlive the panel (a Steam restart takes SteamVR and the panel down with it).
// spawnDetached / detachedArgv / kSpawnEnvironment adapted from frameclash-panel, itself from frameeyeosc's
// panel/src/setup_tools.{h,cpp} (https://github.com/sasaken1102r/frameeyeosc, commit c4294a9).
// Copyright (c) 2026 konsti219; Copyright (c) 2026 sasaken1102r (original). MIT License:
// third_party_frameeyeosc/LICENSE.frameeyeosc. Changes for FrameClash and frame-vrc are under the same license.
#pragma once

#include <string>
#include <vector>

/** What a program started through systemd-run takes along from the panel's environment (only those set). */
constexpr const char* kSpawnEnvironment[] = {"DISPLAY",         "WAYLAND_DISPLAY", "XAUTHORITY",
                                             "XDG_RUNTIME_DIR", "XDG_SESSION_TYPE", "XDG_CURRENT_DESKTOP",
                                             "DBUS_SESSION_BUS_ADDRESS", "LANG", "LC_ALL"};

/** Through "systemd-run --user --collect --quiet -E ..." when available (its own unit, outside SteamVR's). */
std::vector<std::string> detachedArgv(const std::vector<std::string>& argv, bool systemdRun);

bool inPath(const std::string& name);

/** Start it detached (double fork, own session, stdio to /dev/null). */
bool spawnDetached(const std::vector<std::string>& program, std::string& error);

/** Run it in its own process group with stdout captured (stdin, stderr: /dev/null); killed after timeoutMs.
 *  @return its exit code, -1 if it could not run or was killed */
int capture(const std::vector<std::string>& argv, std::string& out, int timeoutMs);
