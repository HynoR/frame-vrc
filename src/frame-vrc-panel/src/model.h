// frame-vrc panel: the data side. A worker thread asks the frame-vrc CLI (status --json, doctor --json) so a slow
// call never stalls the VR frame loop, and runs one action at a time. Actions run detached, their output in a log
// under ~/.local/share/frame-vrc/panel/: they outlive the panel (a Steam restart takes SteamVR and the panel down),
// and a panel opened again picks up the one still running.
// Copyright (c) 2026 FrameClash contributors. MIT License.
#pragma once

#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/** frame-vrc status --json */
struct Status {
    bool known = false;
    std::string version, fex, fexBuild, proton, steamTool, dfr, dfrEffective, video, launch;
    /** frame-vrc next: the step (game, android, download, deps, patch, settings, ready, steam), the arguments that
     *  do it and a second choice ("" = none) */
    std::string nextStep, nextFix, nextAlt;
    std::string build;    ///< VRChat build installed: pc, android or ""
    std::string edition;  ///< official (Valve's setup: the Android build on Lepton), community (the PC build) or ""
    int swapFileMb = 0;       ///< /home/swapfile (SteamOS ships 1 GB; 8 GB through frame-vrc swap 8g)
    int resolution = 0;  ///< 0: SteamVR did not answer
    std::string runtime;  // ji10me's runtime and whether the Vulkan thunks are on
    bool gameRunning = false;
};

/** One frame-vrc doctor --json row; fix = the frame-vrc arguments that fix it ("" = none). */
struct Check {
    std::string group, level, text, fix;
};

/** One step row from frame-vrc setup-status --json. */
struct SetupStepResult {
    int step = 0;
    std::string title, state, note;

    bool operator==(const SetupStepResult& o) const {
        return step == o.step && title == o.title && state == o.state && note == o.note;
    }
    bool operator!=(const SetupStepResult& o) const { return !(*this == o); }
};

/** frame-vrc setup-status --json */
struct SetupProgress {
    bool active = false;
    std::string op, state, title, sub, reason, appId;
    int step = 0, total = 0;
    bool download = false, gamePending = false, alive = false;
    std::vector<std::string> actions;
    std::vector<SetupStepResult> results;

    bool operator==(const SetupProgress& o) const {
        return active == o.active && op == o.op && state == o.state && title == o.title && sub == o.sub &&
               reason == o.reason && appId == o.appId && step == o.step && total == o.total &&
               download == o.download && gamePending == o.gamePending && alive == o.alive && actions == o.actions &&
               results == o.results;
    }
    bool operator!=(const SetupProgress& o) const { return !(*this == o); }
};

struct Snapshot {
    uint64_t seq = 0;              ///< bumps on every change (the UI redraws when it moves)
    Status status;
    SetupProgress setup;
    std::string error;             ///< why the CLI could not be asked
    std::vector<Check> checks;
    bool checked = false;          ///< doctor answered at least once
    bool refreshing = false;
    bool running = false;          ///< an action runs (maybe one an earlier panel started)
    std::string runLabel;          ///< its arguments, e.g. "proton dfr"
    std::string runLine;           ///< its last output line
    std::vector<std::string> runLog;  ///< its output as on a terminal (download bars: only the live one, as "… NN%")
    int64_t runSinceMs = 0;
    /** Settings tapped but not confirmed by status yet (resolution, dfr, cpus, dxvk-threads, vram): waiting for the
     *  taps to settle, being applied, or applied and not refreshed yet. The UI shows these over the status. */
    std::map<std::string, std::string> pending;
    int64_t applyAtMs = 0;         ///< when the waiting ones go out (0: none waiting)
    bool haveResult = false;       ///< the last action's outcome
    bool resultOk = false;
    std::string resultLabel, resultLine;
};

class Model {
public:
    /** @param cli the frame-vrc script @param stateDir where the action's pid, label and log are kept */
    Model(std::string cli, std::string stateDir);
    ~Model();
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    Snapshot snapshot() const;
    void refresh();
    /** Start frame-vrc ARGS (ignored while an action runs). */
    void run(const std::vector<std::string>& args);
    /** Ask a waiting setup task to reopen its current Steam install prompt (runs even while an action runs). */
    void reopen();
    /** A setting tap: sent with the others as one "frame-vrc apply" once no tap came for kApplyDelayMs. */
    void stage(const std::string& key, const std::string& value);
    /** Run doctor on every refresh (the Environment page), else only at start and after actions. */
    void setFullChecks(bool on);
    /** Refresh period: fast while the panel is shown, slow when hidden. */
    void setInterval(int ms);
    /** True when nothing is queued or running. */
    bool idle() const;

private:
    std::string cli_, dir_;
    mutable std::mutex mutex_;
    std::condition_variable wake_;
    std::vector<std::string> pendingRun_;
    std::map<std::string, std::string> staged_, inflight_;
    int64_t applyAt_ = 0;
    bool clearInflight_ = false;   ///< the next refresh shows the applied values: drop inflight_
    bool fullChecks_ = false;
    bool checksDue_ = true;
    bool refreshNow_ = true;
    bool reopenPending_ = false;
    bool quit_ = false;
    int intervalMs_ = 10000;
    int64_t nextRefresh_ = 0;
    int64_t nextSetupPoll_ = 0;
    Snapshot snap_;
    std::thread thread_;

    void loop();
    void doRefresh();
    void startRun(const std::vector<std::string>& args);
    /** Check the action: still running, finished (its exit line) or gone. */
    void pollRun();
    void syncPending();
    void changed() { ++snap_.seq; }
};

int64_t nowMs();
