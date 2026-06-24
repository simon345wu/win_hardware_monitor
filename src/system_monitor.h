#pragma once
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

struct CircularBuffer {
    std::vector<float> data;
    size_t head = 0;
    size_t maxSize = 300;

    void Resize(size_t newSize) {
        data.clear();
        head = 0;
        maxSize = newSize;
    }

    void Add(float val) {
        if (data.size() < maxSize) {
            data.push_back(val);
        } else {
            data[head] = val;
            head = (head + 1) % maxSize;
        }
    }

    std::vector<float> GetContiguous() const {
        std::vector<float> result;
        result.reserve(data.size());
        if (data.size() < maxSize) {
            result.insert(result.end(), data.begin(), data.end());
        } else {
            result.insert(result.end(), data.begin() + head, data.end());
            result.insert(result.end(), data.begin(), data.begin() + head);
        }
        return result;
    }

    float GetLatest() const {
        if (data.empty()) return 0.0f;
        if (data.size() < maxSize) return data.back();
        return data[(head + maxSize - 1) % maxSize];
    }
};

struct MonitorSnapshot {
    std::vector<std::vector<float>> coreHistories; // [coreIndex][timeIndex]
    std::vector<float> avgCpuHistory;
    std::vector<float> memoryHistory;
    
    std::vector<float> currentCoreUsages;
    float currentAvgCpu = 0.0f;
    float currentMemoryPct = 0.0f;
    
    double totalMemoryGB = 0.0;
    double usedMemoryGB = 0.0;
    double availMemoryGB = 0.0;
    
    int sampleCount = 0;
};

class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();

    void Start();
    void Stop();

    // Configuration
    void SetIntervalMs(int ms);
    int GetIntervalMs() const { return m_intervalMs.load(); }

    void SetHistoryLength(int length);
    int GetHistoryLength() const { return m_historyLength.load(); }

    // Info
    int GetCoreCount() const { return m_coreCount; }
    const std::string& GetCpuModelName() const { return m_cpuModelName; }
    
    // Data retrieval
    void GetSnapshot(MonitorSnapshot& snapshot);

private:
    void WorkerLoop();
    void CollectMetrics();
    void QueryCpuModelName();

    // Threading
    std::thread m_workerThread;
    std::atomic<bool> m_running{false};
    std::mutex m_dataMutex;
    std::atomic<int> m_intervalMs{200};
    std::atomic<int> m_historyLength{300};

    // System Info
    int m_coreCount = 0;
    std::string m_cpuModelName;

    // Historical data buffers
    std::vector<CircularBuffer> m_coreHistories;
    CircularBuffer m_avgCpuHistory;
    CircularBuffer m_memoryHistory;

    // Previous CPU state for delta calculation
    struct CpuTimes {
        unsigned long long idleTime = 0;
        unsigned long long kernelTime = 0;
        unsigned long long userTime = 0;
    };
    std::vector<CpuTimes> m_prevCoreTimes;
    CpuTimes m_prevTotalTimes;

    // Memory stats
    double m_totalMemoryGB = 0.0;
    double m_usedMemoryGB = 0.0;
    double m_availMemoryGB = 0.0;
};
