#ifndef SYSTEM_DATA_COLLECTOR_H
#define SYSTEM_DATA_COLLECTOR_H

#include <QThread>
#include <QMutex>
#include <atomic>
#include <unordered_map>
#include <chrono>
#include "metrics.h"

/// QThread-based system data collector that polls system metrics at a
/// configurable interval and emits signals with fresh data.
class SystemDataCollector : public QThread {
    Q_OBJECT

public:
    explicit SystemDataCollector(QObject* parent = nullptr);
    ~SystemDataCollector() override;

    /// Set the polling interval for CPU/memory metrics (milliseconds).
    void setPollingInterval(int ms);
    int pollingInterval() const;

    /// Set the polling interval for process list (milliseconds).
    void setProcessPollingInterval(int ms);
    int processPollingInterval() const;

    /// Request the collector thread to stop.
    void stop();

signals:
    void cpuMetricsUpdated(const CpuMetrics& metrics);
    void memoryMetricsUpdated(const MemoryMetrics& metrics);
    void processListUpdated(const std::vector<ProcessInfo>& processes);
    void snapshotReady(const SystemSnapshot& snapshot);

protected:
    void run() override;

private:
    CpuMetrics collectCpuMetrics();
    MemoryMetrics collectMemoryMetrics();
    std::vector<ProcessInfo> collectProcessList();

    // Platform-specific helpers
#ifdef __APPLE__
    CpuMetrics collectCpuMetrics_macOS();
    MemoryMetrics collectMemoryMetrics_macOS();
    std::vector<ProcessInfo> collectProcessList_macOS();
    // Per-process CPU time tracking for delta-based CPU %
    std::unordered_map<pid_t, uint64_t> m_prevProcessCpuTime;
    std::chrono::steady_clock::time_point m_prevProcessSampleTime;
    bool m_hasProcessBaseline = false;
#elif defined(__linux__)
    CpuMetrics collectCpuMetrics_linux();
    MemoryMetrics collectMemoryMetrics_linux();
    std::vector<ProcessInfo> collectProcessList_linux();
    std::vector<uint64_t> m_prevCpuTimes;
    std::vector<uint64_t> m_prevCpuIdleTimes;
#endif

    std::atomic<bool> m_running;
    std::atomic<int> m_pollingIntervalMs;
    std::atomic<int> m_processPollingIntervalMs;
};

// Register metatypes for signal/slot across threads
Q_DECLARE_METATYPE(CpuMetrics)
Q_DECLARE_METATYPE(MemoryMetrics)
Q_DECLARE_METATYPE(std::vector<ProcessInfo>)
Q_DECLARE_METATYPE(SystemSnapshot)

#endif // SYSTEM_DATA_COLLECTOR_H
