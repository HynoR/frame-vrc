// Copyright (c) 2026 FrameClash contributors. MIT License.
#include "model.h"

#include "spawn.h"
#include "third_party_frameeyeosc/json.h"

#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {

// Runs as: bash -c SCRIPT frame-vrc-action PIDFILE LOG CLI ARGS... (its pid first, then the CLI's output, then its
// exit code as the log's last line)
constexpr const char* kRunScript =
    "echo $$ >\"$1\"; l=$2; shift 2; \"$@\" >\"$l\" 2>&1 </dev/null; echo \"__exit $?\" >>\"$l\"";
constexpr int kQueryTimeoutMs = 60000;
constexpr int64_t kApplyDelayMs = 1500;  ///< quiet time after the last setting tap before it is applied
constexpr int64_t kStartGraceMs = 5000;  ///< a new action may take this long to write its pid

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

/** mkdir -p (the state folder's parents do not exist before frame-vrc's first command) */
void makeDirs(const std::string& path) {
    for (size_t i = 1; i <= path.size(); ++i)
        if (i == path.size() || path[i] == '/') ::mkdir(path.substr(0, i).c_str(), 0700);
}

void writeFile(const std::string& path, const std::string& text) {
    std::ofstream(path) << text;
}

std::string str(const JsonValue& o, const char* key) {
    const JsonValue* v = o.get(key);
    return v != nullptr && v->isString() ? v->text : "";
}

int num(const JsonValue& o, const char* key) {
    const JsonValue* v = o.get(key);
    return v != nullptr && v->isNumber() ? static_cast<int>(v->number) : 0;
}

/** The last line worth showing: curl's progress bar rewrites its line with \r, so the text after the last \r. */
std::string lastLine(const std::string& log, int* exitCode) {
    std::string last;
    std::istringstream in(log);
    for (std::string line; std::getline(in, line);) {
        if (const size_t cr = line.find_last_of('\r'); cr != std::string::npos) line = line.substr(cr + 1);
        if (line.rfind("__exit ", 0) == 0) {
            if (exitCode != nullptr) *exitCode = std::atoi(line.c_str() + 7);
            continue;
        }
        if (line.find_first_not_of(" \t#=-.0123456789%") != std::string::npos) last = line;
    }
    return last;
}

bool progressBar(const std::string& line) {
    return !line.empty() && line.find_first_not_of(" \t#=-O.0123456789%") == std::string::npos;
}

/** The log as a terminal shows it: each line's text after its last \r; curl's bars dropped, except a bar still
 *  moving at the end (shown as its percentage). At most the last 300 lines. */
std::vector<std::string> terminalLines(const std::string& log) {
    std::vector<std::string> out;
    std::istringstream in(log);
    std::string line;
    bool last = false;
    while (std::getline(in, line)) {
        last = in.peek() == EOF;
        std::string shown = line;
        if (const size_t cr = line.find_last_of('\r'); cr != std::string::npos) shown = line.substr(cr + 1);
        if (shown.rfind("__exit ", 0) == 0) continue;
        if (progressBar(shown) || shown.empty()) {
            const size_t pct = shown.rfind('%');
            if (last && pct != std::string::npos) {
                size_t b = pct;
                while (b > 0 && (std::isdigit(static_cast<unsigned char>(shown[b - 1])) || shown[b - 1] == '.')) --b;
                out.push_back("  … " + shown.substr(b, pct - b + 1));
            }
            continue;
        }
        out.push_back(shown);
    }
    if (out.size() > 300) out.erase(out.begin(), out.end() - 300);
    return out;
}

std::string join(const std::vector<std::string>& args) {
    std::string s;
    for (const auto& a : args) s += (s.empty() ? "" : " ") + a;
    return s;
}

}  // namespace

int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

Model::Model(std::string cli, std::string stateDir) : cli_(std::move(cli)), dir_(std::move(stateDir)) {
    // an action an earlier panel started (still running, or finished since: pollRun shows how it ended)
    if (::access((dir_ + "/run.pid").c_str(), R_OK) == 0) {
        snap_.running = true;
        snap_.runLabel = readFile(dir_ + "/run.cmd");
        snap_.runSinceMs = nowMs() - kStartGraceMs;
    }
    thread_ = std::thread([this] { loop(); });
}

Model::~Model() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        quit_ = true;
    }
    wake_.notify_all();
    thread_.join();
}

Snapshot Model::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snap_;
}

void Model::refresh() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        refreshNow_ = true;
    }
    wake_.notify_all();
}

void Model::run(const std::vector<std::string>& args) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (snap_.running || !pendingRun_.empty()) return;
        pendingRun_ = args;
    }
    wake_.notify_all();
}

void Model::stage(const std::string& key, const std::string& value) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        staged_[key] = value;
        applyAt_ = nowMs() + kApplyDelayMs;
        syncPending();
        changed();
    }
    wake_.notify_all();
}

void Model::syncPending() {
    snap_.pending = inflight_;
    for (const auto& kv : staged_) snap_.pending[kv.first] = kv.second;
    snap_.applyAtMs = staged_.empty() ? 0 : applyAt_;
}

void Model::setFullChecks(bool on) {
    std::lock_guard<std::mutex> lock(mutex_);
    fullChecks_ = on;
}

void Model::setInterval(int ms) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        intervalMs_ = ms;
        nextRefresh_ = std::min(nextRefresh_, nowMs() + ms);
    }
    wake_.notify_all();
}

bool Model::idle() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pendingRun_.empty() && staged_.empty() && !refreshNow_ && !snap_.refreshing && !snap_.running;
}

void Model::loop() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!quit_) {
        if (!pendingRun_.empty()) {
            const std::vector<std::string> args = std::move(pendingRun_);
            pendingRun_.clear();
            lock.unlock();
            startRun(args);
            lock.lock();
            continue;
        }
        if (snap_.running) {
            lock.unlock();
            pollRun();
            lock.lock();
            wake_.wait_for(lock, std::chrono::milliseconds(500));
            continue;
        }
        if (!staged_.empty() && nowMs() >= applyAt_) {
            std::vector<std::string> args = {"apply"};
            for (const auto& kv : staged_) {
                args.push_back(kv.first + "=" + kv.second);
                inflight_[kv.first] = kv.second;
            }
            staged_.clear();
            syncPending();
            lock.unlock();
            startRun(args);
            lock.lock();
            continue;
        }
        if (refreshNow_ || nowMs() >= nextRefresh_) {
            refreshNow_ = false;
            snap_.refreshing = true;
            changed();
            lock.unlock();
            doRefresh();
            lock.lock();
            snap_.refreshing = false;
            changed();
            nextRefresh_ = nowMs() + intervalMs_;
            continue;
        }
        const int64_t next = staged_.empty() ? nextRefresh_ : std::min(nextRefresh_, applyAt_);
        wake_.wait_for(lock, std::chrono::milliseconds(std::max<int64_t>(50, next - nowMs())));
    }
}

void Model::doRefresh() {
    std::string out, error;
    Status st;
    int rc = capture({cli_, "status", "--json"}, out, kQueryTimeoutMs);
    JsonValue v;
    if (rc == 0 && parseJson(out, v, error) && v.isObject()) {
        st.known = true;
        st.version = str(v, "version");
        st.fex = str(v, "fex");
        st.fexBuild = str(v, "fex_build");
        st.proton = str(v, "proton");
        st.steamTool = str(v, "steam_tool");
        st.dfr = str(v, "dfr");
        st.dfrEffective = str(v, "dfr_effective");
        st.video = str(v, "video");
        st.launch = str(v, "launch");
        st.nextStep = str(v, "next_step");
        st.nextFix = str(v, "next_fix");
        st.nextAlt = str(v, "next_alt");
        st.build = str(v, "build");
        st.edition = str(v, "edition");
        st.swapFileMb = num(v, "swap_file_mb");
        st.resolution = num(v, "resolution");
        st.runtime = str(v, "runtime");
        const JsonValue* g = v.get("game_running");
        st.gameRunning = g != nullptr && g->isBool() && g->boolean;
        error.clear();
    } else {
        error = rc < 0 ? "frame-vrc status did not answer" : "frame-vrc status failed (exit " + std::to_string(rc) + ")";
    }
    // doctor (a few seconds, asks Steam and the network): not on every refresh unless the Environment page is shown.
    // It exits 1 when something fails; its rows are what counts.
    bool withChecks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        withChecks = fullChecks_ || checksDue_;
        checksDue_ = false;
    }
    std::vector<Check> checks;
    bool checked = false;
    std::string ignored;
    rc = withChecks ? capture({cli_, "doctor", "--json"}, out, kQueryTimeoutMs) : -1;
    if (rc >= 0 && parseJson(out, v, ignored) && v.isArray()) {
        checked = true;
        for (const JsonValue& r : v.items)
            if (r.isObject()) checks.push_back({str(r, "group"), str(r, "level"), str(r, "text"), str(r, "fix")});
    }
    std::lock_guard<std::mutex> lock(mutex_);
    snap_.status = st;
    snap_.error = error;
    if (checked) {
        snap_.checks = std::move(checks);
        snap_.checked = true;
    }
    if (clearInflight_) {
        inflight_.clear();
        clearInflight_ = false;
        syncPending();
    }
    changed();
}

void Model::startRun(const std::vector<std::string>& args) {
    makeDirs(dir_);
    const std::string label = join(args);
    std::remove((dir_ + "/run.pid").c_str());
    writeFile(dir_ + "/run.log", "");
    writeFile(dir_ + "/run.cmd", label);
    std::vector<std::string> argv = {"bash", "-c", kRunScript, "frame-vrc-action", dir_ + "/run.pid", dir_ + "/run.log",
                                     cli_};
    argv.insert(argv.end(), args.begin(), args.end());
    std::string error;
    const bool ok = spawnDetached(argv, error);
    std::lock_guard<std::mutex> lock(mutex_);
    if (ok) {
        snap_.running = true;
        snap_.runLabel = label;
        snap_.runLine = "";
        snap_.runLog.clear();
        snap_.runSinceMs = nowMs();
    } else {
        snap_.haveResult = true;
        snap_.resultOk = false;
        snap_.resultLabel = label;
        snap_.resultLine = "could not start: " + error;
    }
    changed();
}

void Model::pollRun() {
    int exitCode = -1;
    const std::string log = readFile(dir_ + "/run.log");
    const std::string line = lastLine(log, &exitCode);
    std::vector<std::string> lines = terminalLines(log);
    const std::string pidText = readFile(dir_ + "/run.pid");
    const int pid = std::atoi(pidText.c_str());
    int64_t since;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        since = snap_.runSinceMs;
    }
    const bool alive = pid > 0 ? ::kill(pid, 0) == 0 : nowMs() - since < kStartGraceMs;
    std::lock_guard<std::mutex> lock(mutex_);
    if (lines != snap_.runLog) {
        snap_.runLog = std::move(lines);
        changed();
    }
    if (exitCode < 0 && alive) {
        if (line != snap_.runLine) {
            snap_.runLine = line;
            changed();
        }
        return;
    }
    snap_.running = false;
    snap_.haveResult = true;
    snap_.resultOk = exitCode == 0;
    snap_.resultLabel = snap_.runLabel;
    snap_.resultLine = exitCode < 0 ? "stopped before it finished" + (line.empty() ? "" : ": " + line) : line;
    std::remove((dir_ + "/run.pid").c_str());
    refreshNow_ = true;
    checksDue_ = true;
    clearInflight_ = true;
    changed();
}
