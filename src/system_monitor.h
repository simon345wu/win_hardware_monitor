#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

// Number of samples kept per metric (the horizontal span of each chart).
constexpr int kHistoryLength = 120;

struct CircularBuffer {
    std::vector<float> data;
    size_t head = 0;
    size_t maxSize = kHistoryLength;

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
    std::vector<float> cpuHistory;        // %
    std::vector<float> memHistory;        // %
    std::vector<float> diskReadHistory;   // KB/s
    std::vector<float> diskWriteHistory;  // KB/s
    std::vector<float> netDownHistory;    // KB/s
    std::vector<float> netUpHistory;      // KB/s

    float cpu = 0.0f;
    float mem = 0.0f;
    float diskRead = 0.0f;
    float diskWrite = 0.0f;
    float netDown = 0.0f;
    float netUp = 0.0f;

    double totalMemGB = 0.0;
    double usedMemGB = 0.0;
};

class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();

    void Start();
    void Stop();

    void SetIntervalMs(int ms);
    int GetIntervalMs() const { return m_intervalMs.load(); }

    void GetSnapshot(MonitorSnapshot& snapshot);

private:
    void WorkerLoop();
    void Collect();

    std::thread m_workerThread;
    std::atomic<bool> m_running{false};
    std::mutex m_dataMutex;
    std::atomic<int> m_intervalMs{1000};

    CircularBuffer m_cpuHistory;
    CircularBuffer m_memHistory;
    CircularBuffer m_diskReadHistory;
    CircularBuffer m_diskWriteHistory;
    CircularBuffer m_netDownHistory;
    CircularBuffer m_netUpHistory;

    double m_totalMemGB = 0.0;
    double m_usedMemGB = 0.0;

    // Previous sample state for delta computation (worker thread only).
    unsigned long long m_prevIdle = 0;
    unsigned long long m_prevKernel = 0;
    unsigned long long m_prevUser = 0;
    unsigned long long m_prevNetIn = 0;
    unsigned long long m_prevNetOut = 0;
    std::chrono::steady_clock::time_point m_prevSampleTime;
    bool m_hasBaseline = false;

    // PDH handles, kept as void* so this header stays free of windows.h.
    void* m_pdhQuery = nullptr;
    void* m_diskReadCounter = nullptr;
    void* m_diskWriteCounter = nullptr;
};
