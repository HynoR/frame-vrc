// frame-vrc panel: system load for the Monitor page. Read-only and cheap (about 1 ms): /proc/meminfo, /proc/stat,
// zram's mm_stat, thermal zones, the GPU's devfreq clock, and VRChat.exe's /proc/PID/status and stat (never its
// smaps: reading the game's full smaps makes it stutter).
// Copyright (c) 2026 FrameClash contributors. MIT License.
#pragma once

#include <cstdint>
#include <deque>
#include <vector>

struct MonitorSample {
    bool valid = false;
    double memTotalMb = 0, memAvailMb = 0;
    double swapTotalMb = 0, swapUsedMb = 0;
    double zramDataMb = 0, zramMemMb = 0;   ///< data swapped to zram / RAM it takes compressed
    double cpuPct = 0;                      ///< all cores, 0-100
    std::vector<double> corePct;
    double tempC = 0;                       ///< the hottest thermal zone
    double gpuMhz = 0, gpuMaxMhz = 0;
    int gamePid = 0;                        ///< VRChat.exe, 0 = not running
    double gameRssMb = 0, gameCores = 0;    ///< its resident memory, CPU in cores
};

class SystemMonitor {
public:
    static constexpr size_t kHistory = 120;  ///< samples kept for the graphs

    /** Take a sample (call about once a second; rates come from the time since the last one). */
    void sample();
    const MonitorSample& last() const { return last_; }
    const std::deque<double>& memHistory() const { return mem_; }   ///< used RAM, % of total
    const std::deque<double>& cpuHistory() const { return cpu_; }   ///< all cores, %

private:
    MonitorSample last_;
    std::vector<uint64_t> prevBusy_, prevTotal_;   ///< per /proc/stat cpu line (0 = all)
    uint64_t prevGameTicks_ = 0;
    int prevGamePid_ = 0;
    int64_t prevAtMs_ = 0;
    std::deque<double> mem_, cpu_;
};
