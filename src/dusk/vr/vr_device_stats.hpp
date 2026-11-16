#pragma once

// TEMP DIAGNOSTIC (TPVR Quest 1 rendering cost, 2026-11-16). Remove once the
// costly effects are found and fixed.
//
// Logs the headset's own state, so a slow stretch in the log can be told
// apart from the GPU or CPU clocking down or the headset getting hot:
// - once at start: GPU model, its clock steps and governor, each CPU's top
//   clock and governor, every temperature sensor's name, and every file
//   that could not be read (with the error, e.g. a permission refusal);
// - once a second ("[dusk::vr::device]"): GPU clock, GPU busy (from the
//   driver's busy/total counters), the GPU's thermal limit level, each CPU's
//   clock, and every temperature sensor.
// Runs on its own thread so slow sensor reads never land in a frame.
// Linux/Android sysfs only; a no-op elsewhere.

#if defined(__linux__)

#include <aurora/lib/logging.hpp>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace dusk::vr::device_stats {
namespace detail {
inline constexpr aurora::Module Log{"dusk::vr"};
inline constexpr const char* Kgsl = "/sys/class/kgsl/kgsl-3d0/";

struct Reading {
    bool ok = false;
    std::string text;
    int err = 0;
};

// Whole file, trailing whitespace removed.
inline Reading readFile(const std::string& path) {
    Reading r;
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        r.err = errno;
        return r;
    }
    char chunk[512];
    for (;;) {
        const ssize_t n = read(fd, chunk, sizeof(chunk));
        if (n < 0) {
            r.err = errno;
            close(fd);
            return r;
        }
        if (n == 0) {
            break;
        }
        r.text.append(chunk, static_cast<size_t>(n));
    }
    close(fd);
    while (!r.text.empty() && (r.text.back() == '\n' || r.text.back() == ' ' || r.text.back() == '\t')) {
        r.text.pop_back();
    }
    r.ok = true;
    return r;
}

// Logs the value, or why it could not be read.
inline std::string describe(const std::string& path) {
    const Reading r = readFile(path);
    if (r.ok) {
        return r.text;
    }
    return std::string("unreadable (") + std::strerror(r.err) + ")";
}

struct Zone {
    std::string type;
    std::string tempPath;
};

inline std::vector<Zone> findZones() {
    std::vector<Zone> zones;
    DIR* dir = opendir("/sys/class/thermal");
    if (dir == nullptr) {
        Log.info("[dusk::vr::device] /sys/class/thermal unreadable ({})", std::strerror(errno));
        return zones;
    }
    while (const dirent* entry = readdir(dir)) {
        if (std::strncmp(entry->d_name, "thermal_zone", 12) != 0) {
            continue;
        }
        const std::string base = std::string("/sys/class/thermal/") + entry->d_name + "/";
        const Reading type = readFile(base + "type");
        zones.push_back({type.ok ? type.text : std::string(entry->d_name), base + "temp"});
    }
    closedir(dir);
    std::sort(zones.begin(), zones.end(), [](const Zone& a, const Zone& b) { return a.type < b.type; });
    return zones;
}

inline int cpuCount() {
    int n = 0;
    while (access(("/sys/devices/system/cpu/cpu" + std::to_string(n)).c_str(), F_OK) == 0) {
        ++n;
    }
    return n;
}

inline std::string cpuPath(int cpu, const char* file) {
    return "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cpufreq/" + file;
}

// Sysfs temperatures are millidegrees on most sensors, whole degrees on a few.
inline std::string formatTemp(const std::string& raw) {
    char* end = nullptr;
    const long v = std::strtol(raw.c_str(), &end, 10);
    if (end == raw.c_str()) {
        return raw;
    }
    const double c = (v >= 1000 || v <= -1000) ? static_cast<double>(v) / 1000.0 : static_cast<double>(v);
    return fmt::format("{:.1f}", c);
}

inline void logStatic(const std::vector<Zone>& zones, int cpus) {
    const std::string k = Kgsl;
    Log.info("[dusk::vr::device] GPU {} | clock steps (Hz): {} | max clock {} | power levels min {} max {} | "
             "devfreq governor {} | thermal level {}",
             describe(k + "gpu_model"), describe(k + "gpu_available_frequencies"), describe(k + "max_gpuclk"),
             describe(k + "min_pwrlevel"), describe(k + "max_pwrlevel"), describe(k + "devfreq/governor"),
             describe(k + "thermal_pwrlevel"));
    for (int cpu = 0; cpu < cpus; ++cpu) {
        Log.info("[dusk::vr::device] CPU{}: max {} kHz, min {} kHz, governor {}", cpu,
                 describe(cpuPath(cpu, "cpuinfo_max_freq")), describe(cpuPath(cpu, "scaling_min_freq")),
                 describe(cpuPath(cpu, "scaling_governor")));
    }
    std::string names;
    for (const Zone& z : zones) {
        const Reading r = readFile(z.tempPath);
        names += ' ';
        names += z.type;
        if (!r.ok) {
            names += std::string("(unreadable: ") + std::strerror(r.err) + ")";
        }
    }
    Log.info("[dusk::vr::device] {} temperature sensors:{}", zones.size(), names);
}

inline void sampleLoop() {
    const std::vector<Zone> zones = findZones();
    const int cpus = cpuCount();
    logStatic(zones, cpus);

    const std::string k = Kgsl;
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // gpubusy: the driver's "busy total" counters for its latest window.
        std::string busy = "n/a";
        const Reading gb = readFile(k + "gpubusy");
        if (gb.ok) {
            long long b = 0, t = 0;
            if (std::sscanf(gb.text.c_str(), "%lld %lld", &b, &t) == 2 && t > 0) {
                busy = fmt::format("{:.0f}%", 100.0 * static_cast<double>(b) / static_cast<double>(t));
            }
        } else {
            busy = describe(k + "gpu_busy_percentage");
        }

        std::string cpuClocks;
        for (int cpu = 0; cpu < cpus; ++cpu) {
            const Reading r = readFile(cpuPath(cpu, "scaling_cur_freq"));
            cpuClocks += ' ';
            cpuClocks += r.ok ? std::to_string(std::atoll(r.text.c_str()) / 1000) : std::string("off");
        }

        std::string temps;
        for (const Zone& z : zones) {
            const Reading r = readFile(z.tempPath);
            if (!r.ok) {
                continue;
            }
            temps += ' ';
            temps += z.type;
            temps += '=';
            temps += formatTemp(r.text);
        }

        const Reading clk = readFile(k + "gpuclk");
        Log.info("[dusk::vr::device] GPU {} MHz, busy {}, thermal level {} | CPU MHz:{} | temps C:{}",
                 clk.ok ? std::to_string(std::atoll(clk.text.c_str()) / 1000000) : describe(k + "gpuclk"), busy,
                 describe(k + "thermal_pwrlevel"), cpuClocks, temps);
    }
}
}  // namespace detail

// Starts the once-a-second sampler; later calls do nothing.
inline void start() {
    static bool started = false;
    if (started) {
        return;
    }
    started = true;
    std::thread(detail::sampleLoop).detach();
}
}  // namespace dusk::vr::device_stats

#else

namespace dusk::vr::device_stats {
inline void start() {}
}  // namespace dusk::vr::device_stats

#endif
