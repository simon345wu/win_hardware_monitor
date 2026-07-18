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
    float cpuTemp = 0.0f;
    bool isRealTemp = false;
    float memTemp = 0.0f;   // hottest DIMM, °C (DDR5 SPD5118 sensor)
    bool hasMemTemp = false;

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
    void Collect(void* pSvc = nullptr);

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
    float m_cpuTemp = 0.0f;
    bool m_isRealTemp = false;
    float m_memTemp = 0.0f;
    bool m_hasMemTemp = false;

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

    // WinRing0 members and helper functions
    void InitWinRing0();
    void ShutdownWinRing0();
    bool IsAmdCpu();
    bool IsIntelCpu();

    // DIMM temperature over the FCH SMBus controller (AMD; DDR5 SPD5118 hubs).
    void InitSmbus();
    bool SmbusReadByte(unsigned char devAddr, unsigned char reg, unsigned char& value);
    bool ReadDimmTemp(unsigned char devAddr, float& tempC);

    unsigned short m_smbusBase = 0;      // SMBus host controller I/O base; 0 = unavailable
    unsigned char m_dimmAddrs[8] = {};   // detected SPD5118 SMBus addresses
    int m_dimmCount = 0;
    void* m_smbusMutex = nullptr;        // HANDLE; industry-convention SMBus arbitration mutex

    void* m_hWinRing0 = nullptr; // HMODULE
    bool m_winRing0Ready = false;

    typedef int (__stdcall *InitializeOlsFunc)();
    typedef void (__stdcall *DeinitializeOlsFunc)();
    typedef unsigned long (__stdcall *GetDllStatusFunc)();
    typedef int (__stdcall *ReadPciConfigDwordExFunc)(unsigned long pciAddress, unsigned long regAddress, unsigned long* value);
    typedef int (__stdcall *WritePciConfigDwordExFunc)(unsigned long pciAddress, unsigned long regAddress, unsigned long value);
    typedef int (__stdcall *RdmsrFunc)(unsigned long msr, unsigned long* eax, unsigned long* edx);
    typedef unsigned char (__stdcall *ReadIoPortByteFunc)(unsigned short port);
    typedef void (__stdcall *WriteIoPortByteFunc)(unsigned short port, unsigned char value);

    InitializeOlsFunc m_InitializeOls = nullptr;
    DeinitializeOlsFunc m_DeinitializeOls = nullptr;
    GetDllStatusFunc m_GetDllStatus = nullptr;
    ReadPciConfigDwordExFunc m_ReadPciConfigDwordEx = nullptr;
    WritePciConfigDwordExFunc m_WritePciConfigDwordEx = nullptr;
    RdmsrFunc m_Rdmsr = nullptr;
    ReadIoPortByteFunc m_ReadIoPortByte = nullptr;
    WriteIoPortByteFunc m_WriteIoPortByte = nullptr;
};
