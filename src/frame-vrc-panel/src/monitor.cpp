// Copyright (c) 2026 FrameClash contributors. MIT License.
#include "monitor.h"

#include "model.h"

#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

namespace {

double readNumber(const std::string& path) {
    std::ifstream f(path);
    double v = 0;
    f >> v;
    return v;
}

int findGame() {
    DIR* d = ::opendir("/proc");
    if (d == nullptr) return 0;
    int pid = 0;
    while (dirent* e = ::readdir(d)) {
        if (e->d_name[0] < '1' || e->d_name[0] > '9') continue;
        std::ifstream f(std::string("/proc/") + e->d_name + "/comm");
        std::string comm;
        if (std::getline(f, comm) && comm == "VRChat.exe") {
            pid = std::atoi(e->d_name);
            break;
        }
    }
    ::closedir(d);
    return pid;
}

}  // namespace

void SystemMonitor::sample() {
    MonitorSample s;
    const int64_t now = nowMs();
    {
        std::ifstream f("/proc/meminfo");
        std::string key;
        double kb;
        std::string unit;
        double swapFree = 0;
        while (f >> key >> kb) {
            std::getline(f, unit);
            if (key == "MemTotal:") s.memTotalMb = kb / 1024;
            else if (key == "MemAvailable:") s.memAvailMb = kb / 1024;
            else if (key == "SwapTotal:") s.swapTotalMb = kb / 1024;
            else if (key == "SwapFree:") swapFree = kb / 1024;
        }
        s.swapUsedMb = s.swapTotalMb - swapFree;
    }
    {
        std::ifstream f("/sys/block/zram0/mm_stat");
        double orig = 0, compr = 0, used = 0;
        if (f >> orig >> compr >> used) {
            s.zramDataMb = orig / 1048576;
            s.zramMemMb = used / 1048576;
        }
    }
    {
        // cpu  user nice system idle iowait irq softirq steal ...: busy = all but idle and iowait
        std::ifstream f("/proc/stat");
        std::vector<uint64_t> busy, total;
        for (std::string line; std::getline(f, line) && line.rfind("cpu", 0) == 0;) {
            std::istringstream in(line.substr(line.find(' ')));
            uint64_t v, sum = 0, idle = 0;
            for (int i = 0; in >> v && i < 8; ++i) {
                sum += v;
                if (i == 3 || i == 4) idle += v;
            }
            busy.push_back(sum - idle);
            total.push_back(sum);
        }
        if (prevTotal_.size() == total.size()) {
            for (size_t i = 0; i < total.size(); ++i) {
                const double dt = static_cast<double>(total[i] - prevTotal_[i]);
                const double pct = dt > 0 ? 100.0 * static_cast<double>(busy[i] - prevBusy_[i]) / dt : 0;
                if (i == 0) s.cpuPct = pct;
                else s.corePct.push_back(pct);
            }
        }
        prevBusy_ = busy;
        prevTotal_ = total;
    }
    for (int i = 0; i < 64; ++i) {
        const std::string p = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/temp";
        if (::access(p.c_str(), R_OK) != 0) break;
        s.tempC = std::max(s.tempC, readNumber(p) / 1000);
    }
    if (DIR* d = ::opendir("/sys/class/devfreq")) {
        while (dirent* e = ::readdir(d))
            if (std::strstr(e->d_name, "gpu") != nullptr) {
                const std::string base = std::string("/sys/class/devfreq/") + e->d_name;
                s.gpuMhz = readNumber(base + "/cur_freq") / 1e6;
                s.gpuMaxMhz = readNumber(base + "/max_freq") / 1e6;
            }
        ::closedir(d);
    }
    s.gamePid = findGame();
    if (s.gamePid > 0) {
        const std::string base = "/proc/" + std::to_string(s.gamePid);
        std::ifstream status(base + "/status");
        for (std::string line; std::getline(status, line);)
            if (line.rfind("VmRSS:", 0) == 0) s.gameRssMb = std::atof(line.c_str() + 6) / 1024;
        // stat: utime and stime are fields 14 and 15, counted after the ")" that ends the name
        std::ifstream stat(base + "/stat");
        std::string text((std::istreambuf_iterator<char>(stat)), std::istreambuf_iterator<char>());
        const size_t close = text.rfind(')');
        if (close != std::string::npos) {
            std::istringstream in(text.substr(close + 2));
            std::string field;
            uint64_t ticks = 0;
            for (int i = 3; in >> field && i <= 15; ++i)
                if (i >= 14) ticks += std::strtoull(field.c_str(), nullptr, 10);
            if (s.gamePid == prevGamePid_ && prevAtMs_ > 0 && now > prevAtMs_)
                s.gameCores = static_cast<double>(ticks - prevGameTicks_) / ::sysconf(_SC_CLK_TCK) /
                              ((now - prevAtMs_) / 1000.0);
            prevGameTicks_ = ticks;
        }
    }
    prevGamePid_ = s.gamePid;
    prevAtMs_ = now;
    s.valid = s.memTotalMb > 0;
    if (s.valid) {
        mem_.push_back(100.0 * (s.memTotalMb - s.memAvailMb) / s.memTotalMb);
        cpu_.push_back(s.cpuPct);
        while (mem_.size() > kHistory) mem_.pop_front();
        while (cpu_.size() > kHistory) cpu_.pop_front();
    }
    last_ = s;
}
