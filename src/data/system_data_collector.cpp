#include "system_data_collector.h"
#include <QDebug>
#include <chrono>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>
#include <libproc.h>
#include <unistd.h>
#endif

#ifdef __linux__
#include <unistd.h>
#include <dirent.h>
#endif

SystemDataCollector::SystemDataCollector(QObject* parent)
    : QThread(parent)
    , m_running(false)
    , m_pollingIntervalMs(1000)
    , m_processPollingIntervalMs(2000)
{
    qRegisterMetaType<CpuMetrics>();
    qRegisterMetaType<MemoryMetrics>();
    qRegisterMetaType<std::vector<ProcessInfo>>();
    qRegisterMetaType<SystemSnapshot>();
}

SystemDataCollector::~SystemDataCollector() {
    stop();
    if (isRunning()) {
        wait(5000);
    }
}

void SystemDataCollector::setPollingInterval(int ms) {
    m_pollingIntervalMs.store(std::max(100, ms));
}

int SystemDataCollector::pollingInterval() const {
    return m_pollingIntervalMs.load();
}

void SystemDataCollector::setProcessPollingInterval(int ms) {
    m_processPollingIntervalMs.store(std::max(500, ms));
}

int SystemDataCollector::processPollingInterval() const {
    return m_processPollingIntervalMs.load();
}

void SystemDataCollector::stop() {
    m_running.store(false);
}

void SystemDataCollector::run() {
    m_running.store(true);

    auto lastProcessPoll = std::chrono::steady_clock::now();
    auto processPollInterval = std::chrono::milliseconds(m_processPollingIntervalMs.load());

    while (m_running.load()) {
        auto startTime = std::chrono::steady_clock::now();

        // Collect CPU and memory every cycle
        CpuMetrics cpu = collectCpuMetrics();
        MemoryMetrics memory = collectMemoryMetrics();

        emit cpuMetricsUpdated(cpu);
        emit memoryMetricsUpdated(memory);

        // Collect process list less frequently
        auto now = std::chrono::steady_clock::now();
        processPollInterval = std::chrono::milliseconds(m_processPollingIntervalMs.load());
        std::vector<ProcessInfo> processes;
        if (now - lastProcessPoll >= processPollInterval) {
            processes = collectProcessList();
            emit processListUpdated(processes);
            lastProcessPoll = now;
        }

        // Emit full snapshot
        SystemSnapshot snapshot;
        snapshot.timestamp = now;
        snapshot.cpu = cpu;
        snapshot.memory = memory;
        snapshot.processes = processes;
        emit snapshotReady(snapshot);

        // Sleep for the remainder of the polling interval
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        auto sleepTime = std::chrono::milliseconds(m_pollingIntervalMs.load()) - elapsed;
        if (sleepTime.count() > 0) {
            QThread::msleep(
                static_cast<unsigned long>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(sleepTime).count()));
        }
    }
}

// ---- Dispatcher to platform-specific implementations ----

CpuMetrics SystemDataCollector::collectCpuMetrics() {
#ifdef __APPLE__
    return collectCpuMetrics_macOS();
#elif defined(__linux__)
    return collectCpuMetrics_linux();
#else
    // Fallback: return empty metrics
    CpuMetrics m;
    m.core_count = 1;
    m.total_usage = 0.0;
    m.per_core = {0.0};
    return m;
#endif
}

MemoryMetrics SystemDataCollector::collectMemoryMetrics() {
#ifdef __APPLE__
    return collectMemoryMetrics_macOS();
#elif defined(__linux__)
    return collectMemoryMetrics_linux();
#else
    return MemoryMetrics();
#endif
}

std::vector<ProcessInfo> SystemDataCollector::collectProcessList() {
#ifdef __APPLE__
    return collectProcessList_macOS();
#elif defined(__linux__)
    return collectProcessList_linux();
#else
    return {};
#endif
}

// ---- macOS implementations ----
#ifdef __APPLE__

CpuMetrics SystemDataCollector::collectCpuMetrics_macOS() {
    CpuMetrics metrics;

    // Get number of CPUs
    int ncpu = 0;
    size_t len = sizeof(ncpu);
    sysctlbyname("hw.ncpu", &ncpu, &len, nullptr, 0);
    metrics.core_count = ncpu;

    // Get CPU load info per processor
    processor_info_array_t cpuInfo;
    mach_msg_type_number_t numCpuInfo;
    natural_t numProcessors = 0;

    kern_return_t kr = host_processor_info(
        mach_host_self(),
        PROCESSOR_CPU_LOAD_INFO,
        &numProcessors,
        &cpuInfo,
        &numCpuInfo
    );

    if (kr == KERN_SUCCESS) {
        double totalUser = 0, totalSystem = 0, totalIdle = 0;

        for (natural_t i = 0; i < numProcessors; ++i) {
            double user   = cpuInfo[CPU_STATE_MAX * i + CPU_STATE_USER];
            double system = cpuInfo[CPU_STATE_MAX * i + CPU_STATE_SYSTEM];
            double idle   = cpuInfo[CPU_STATE_MAX * i + CPU_STATE_IDLE];
            double nice   = cpuInfo[CPU_STATE_MAX * i + CPU_STATE_NICE];

            double total = user + system + idle + nice;
            double usage = total > 0 ? ((user + system + nice) / total) * 100.0 : 0.0;
            metrics.per_core.push_back(usage);

            totalUser += user + nice;
            totalSystem += system;
            totalIdle += idle;
        }

        double grandTotal = totalUser + totalSystem + totalIdle;
        metrics.total_usage = grandTotal > 0
            ? ((totalUser + totalSystem) / grandTotal) * 100.0
            : 0.0;

        vm_deallocate(mach_task_self(),
                      reinterpret_cast<vm_address_t>(cpuInfo),
                      numCpuInfo * sizeof(integer_t));
    }

    metrics.average = metrics.total_usage;
    metrics.peak = *std::max_element(metrics.per_core.begin(), metrics.per_core.end());

    return metrics;
}

MemoryMetrics SystemDataCollector::collectMemoryMetrics_macOS() {
    MemoryMetrics metrics;

    // Total physical memory
    int64_t totalMem = 0;
    size_t len = sizeof(totalMem);
    sysctlbyname("hw.memsize", &totalMem, &len, nullptr, 0);
    metrics.total_bytes = static_cast<uint64_t>(totalMem);

    // VM statistics
    vm_size_t pageSize;
    host_page_size(mach_host_self(), &pageSize);

    vm_statistics64_data_t vmStats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    kern_return_t kr = host_statistics64(
        mach_host_self(),
        HOST_VM_INFO64,
        reinterpret_cast<host_info64_t>(&vmStats),
        &count
    );

    if (kr == KERN_SUCCESS) {
        uint64_t active   = static_cast<uint64_t>(vmStats.active_count) * pageSize;
        uint64_t inactive = static_cast<uint64_t>(vmStats.inactive_count) * pageSize;
        uint64_t wired    = static_cast<uint64_t>(vmStats.wire_count) * pageSize;
        uint64_t free_mem = static_cast<uint64_t>(vmStats.free_count) * pageSize;
        uint64_t speculative = static_cast<uint64_t>(vmStats.speculative_count) * pageSize;

        metrics.used_bytes = active + wired;
        metrics.cached_bytes = inactive + speculative;
        metrics.free_bytes = free_mem;
    }

    // Swap usage via sysctl
    struct xsw_usage swapUsage;
    len = sizeof(swapUsage);
    if (sysctlbyname("vm.swapusage", &swapUsage, &len, nullptr, 0) == 0) {
        metrics.swap_total_bytes = swapUsage.xsu_total;
        metrics.swap_used_bytes = swapUsage.xsu_used;
    }

    return metrics;
}

std::vector<ProcessInfo> SystemDataCollector::collectProcessList_macOS() {
    std::vector<ProcessInfo> processes;

    int bufferSize = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
    if (bufferSize <= 0) return processes;

    std::vector<pid_t> pids(bufferSize / sizeof(pid_t));
    proc_listpids(PROC_ALL_PIDS, 0, pids.data(), bufferSize);

    // Get total physical memory for percentage calculation
    int64_t totalMem = 0;
    size_t len = sizeof(totalMem);
    sysctlbyname("hw.memsize", &totalMem, &len, nullptr, 0);

    auto now = std::chrono::steady_clock::now();
    std::unordered_map<pid_t, uint64_t> currentCpuTimes;

    for (pid_t pid : pids) {
        if (pid == 0) continue;

        struct proc_taskallinfo taskInfo;
        int ret = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0,
                               &taskInfo, sizeof(taskInfo));
        if (ret <= 0) continue;

        ProcessInfo info;
        info.pid = pid;
        info.name = std::string(taskInfo.pbsd.pbi_comm);
        info.memory_bytes = taskInfo.ptinfo.pti_resident_size;
        info.memory_percent = totalMem > 0
            ? (static_cast<double>(info.memory_bytes) / totalMem) * 100.0
            : 0.0;

        // Compute CPU % from delta of total CPU time between samples
        uint64_t totalTime = taskInfo.ptinfo.pti_total_user
                           + taskInfo.ptinfo.pti_total_system;
        info.cpu_percent = 0.0;
        if (m_hasProcessBaseline) {
            auto it = m_prevProcessCpuTime.find(pid);
            if (it != m_prevProcessCpuTime.end()) {
                uint64_t deltaTime = totalTime - it->second;  // nanoseconds of CPU used
                double elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    now - m_prevProcessSampleTime).count();
                if (elapsedNs > 0) {
                    info.cpu_percent = (static_cast<double>(deltaTime) / elapsedNs) * 100.0;
                }
            }
        }
        currentCpuTimes[pid] = totalTime;

        // Process status
        switch (taskInfo.pbsd.pbi_status) {
            case SRUN:    info.status = "Running";  break;
            case SSLEEP:  info.status = "Sleeping"; break;
            case SSTOP:   info.status = "Stopped";  break;
            case SZOMB:   info.status = "Zombie";   break;
            default:      info.status = "Unknown";  break;
        }

        processes.push_back(std::move(info));
    }

    // Update baseline for next delta calculation
    m_prevProcessCpuTime = std::move(currentCpuTimes);
    m_prevProcessSampleTime = now;
    m_hasProcessBaseline = true;

    // Sort by CPU usage descending
    std::sort(processes.begin(), processes.end(),
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.cpu_percent > b.cpu_percent;
              });

    // Limit to top 100
    if (processes.size() > 100) {
        processes.resize(100);
    }

    return processes;
}

#endif // __APPLE__

// ---- Linux implementations ----
#ifdef __linux__

CpuMetrics SystemDataCollector::collectCpuMetrics_linux() {
    CpuMetrics metrics;

    std::ifstream file("/proc/stat");
    if (!file.is_open()) return metrics;

    std::string line;
    std::vector<uint64_t> cpuTimes;
    std::vector<uint64_t> cpuIdleTimes;
    int coreIndex = -1;

    while (std::getline(file, line)) {
        if (line.substr(0, 3) != "cpu") break;

        std::istringstream iss(line);
        std::string label;
        iss >> label;

        uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
        iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

        uint64_t totalTime = user + nice + system + idle + iowait + irq + softirq + steal;
        uint64_t idleTime = idle + iowait;

        cpuTimes.push_back(totalTime);
        cpuIdleTimes.push_back(idleTime);

        ++coreIndex;
    }

    metrics.core_count = std::max(0, coreIndex);  // exclude "cpu" aggregate line

    // Calculate usage deltas against previous readings
    if (!m_prevCpuTimes.empty() && m_prevCpuTimes.size() == cpuTimes.size()) {
        for (size_t i = 0; i < cpuTimes.size(); ++i) {
            uint64_t totalDelta = cpuTimes[i] - m_prevCpuTimes[i];
            uint64_t idleDelta = cpuIdleTimes[i] - m_prevCpuIdleTimes[i];
            double usage = totalDelta > 0
                ? (1.0 - static_cast<double>(idleDelta) / totalDelta) * 100.0
                : 0.0;

            if (i == 0) {
                metrics.total_usage = usage;
            } else {
                metrics.per_core.push_back(usage);
            }
        }
    } else {
        // First reading: return zeros
        metrics.total_usage = 0.0;
        for (int i = 0; i < metrics.core_count; ++i) {
            metrics.per_core.push_back(0.0);
        }
    }

    m_prevCpuTimes = cpuTimes;
    m_prevCpuIdleTimes = cpuIdleTimes;

    metrics.average = metrics.total_usage;
    if (!metrics.per_core.empty()) {
        metrics.peak = *std::max_element(metrics.per_core.begin(), metrics.per_core.end());
    }

    return metrics;
}

MemoryMetrics SystemDataCollector::collectMemoryMetrics_linux() {
    MemoryMetrics metrics;

    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return metrics;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        uint64_t value;
        iss >> key >> value;

        // Values in /proc/meminfo are in kB
        if (key == "MemTotal:")        metrics.total_bytes = value * 1024;
        else if (key == "MemFree:")    metrics.free_bytes = value * 1024;
        else if (key == "Cached:")     metrics.cached_bytes = value * 1024;
        else if (key == "SwapTotal:")  metrics.swap_total_bytes = value * 1024;
        else if (key == "SwapFree:")   metrics.swap_used_bytes = value * 1024; // will fix below
    }

    metrics.used_bytes = metrics.total_bytes - metrics.free_bytes - metrics.cached_bytes;
    metrics.swap_used_bytes = metrics.swap_total_bytes - metrics.swap_used_bytes;  // SwapFree was stored

    return metrics;
}

std::vector<ProcessInfo> SystemDataCollector::collectProcessList_linux() {
    std::vector<ProcessInfo> processes;

    DIR* procDir = opendir("/proc");
    if (!procDir) return processes;

    // Get total memory for percentage calculation
    uint64_t totalMem = 0;
    {
        std::ifstream meminfo("/proc/meminfo");
        std::string key;
        uint64_t val;
        if (meminfo >> key >> val) {
            totalMem = val * 1024;  // kB to bytes
        }
    }

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        // Only look at numeric directory names (PIDs)
        if (entry->d_type != DT_DIR) continue;
        bool isNumeric = true;
        for (const char* p = entry->d_name; *p; ++p) {
            if (*p < '0' || *p > '9') { isNumeric = false; break; }
        }
        if (!isNumeric) continue;

        int pid = std::atoi(entry->d_name);
        std::string basePath = std::string("/proc/") + entry->d_name;

        ProcessInfo info;
        info.pid = pid;

        // Read /proc/[pid]/stat
        std::ifstream statFile(basePath + "/stat");
        if (statFile.is_open()) {
            std::string statLine;
            std::getline(statFile, statLine);

            // Parse comm (name in parentheses)
            auto openParen = statLine.find('(');
            auto closeParen = statLine.rfind(')');
            if (openParen != std::string::npos && closeParen != std::string::npos) {
                info.name = statLine.substr(openParen + 1, closeParen - openParen - 1);

                // Fields after the closing paren
                std::istringstream iss(statLine.substr(closeParen + 2));
                char state;
                iss >> state;

                switch (state) {
                    case 'R': info.status = "Running";  break;
                    case 'S': info.status = "Sleeping"; break;
                    case 'T': info.status = "Stopped";  break;
                    case 'Z': info.status = "Zombie";   break;
                    default:  info.status = "Unknown";  break;
                }
            }
        }

        // Read /proc/[pid]/statm for memory
        std::ifstream statmFile(basePath + "/statm");
        if (statmFile.is_open()) {
            uint64_t sizePages, residentPages;
            statmFile >> sizePages >> residentPages;
            long pageSize = sysconf(_SC_PAGESIZE);
            info.memory_bytes = residentPages * pageSize;
            info.memory_percent = totalMem > 0
                ? (static_cast<double>(info.memory_bytes) / totalMem) * 100.0
                : 0.0;
        }

        processes.push_back(std::move(info));
    }
    closedir(procDir);

    // Sort by memory usage descending
    std::sort(processes.begin(), processes.end(),
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.memory_percent > b.memory_percent;
              });

    if (processes.size() > 100) {
        processes.resize(100);
    }

    return processes;
}

#endif // __linux__
