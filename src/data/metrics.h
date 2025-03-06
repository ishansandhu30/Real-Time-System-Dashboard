#ifndef METRICS_H
#define METRICS_H

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

struct CpuMetrics {
    double total_usage;              // Overall CPU usage percentage (0-100)
    std::vector<double> per_core;    // Per-core usage percentages
    double average;                  // Running average
    double peak;                     // Peak value in current window
    int core_count;

    CpuMetrics() : total_usage(0.0), average(0.0), peak(0.0), core_count(0) {}
};

struct MemoryMetrics {
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t free_bytes;
    uint64_t cached_bytes;
    uint64_t swap_total_bytes;
    uint64_t swap_used_bytes;

    double used_percent() const {
        return total_bytes > 0 ? (static_cast<double>(used_bytes) / total_bytes) * 100.0 : 0.0;
    }

    double swap_percent() const {
        return swap_total_bytes > 0 ? (static_cast<double>(swap_used_bytes) / swap_total_bytes) * 100.0 : 0.0;
    }

    double cached_percent() const {
        return total_bytes > 0 ? (static_cast<double>(cached_bytes) / total_bytes) * 100.0 : 0.0;
    }

    MemoryMetrics()
        : total_bytes(0), used_bytes(0), free_bytes(0), cached_bytes(0),
          swap_total_bytes(0), swap_used_bytes(0) {}
};

struct ProcessInfo {
    int pid;
    std::string name;
    double cpu_percent;
    double memory_percent;
    uint64_t memory_bytes;
    std::string status;      // "Running", "Sleeping", "Stopped", "Zombie"
    std::string user;

    ProcessInfo()
        : pid(0), cpu_percent(0.0), memory_percent(0.0), memory_bytes(0) {}
};

struct SystemSnapshot {
    using TimePoint = std::chrono::steady_clock::time_point;

    TimePoint timestamp;
    CpuMetrics cpu;
    MemoryMetrics memory;
    std::vector<ProcessInfo> processes;

    SystemSnapshot() : timestamp(std::chrono::steady_clock::now()) {}
};

#endif // METRICS_H
