#include "system_monitor.h"
#include <windows.h>
#include <chrono>

// Define NtQuerySystemInformation structures
#define SystemProcessorPerformanceInformation 8

typedef LONG NTSTATUS;
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG Reserved1;
    ULONG Reserved2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

typedef NTSTATUS (WINAPI *PNtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

SystemMonitor::SystemMonitor() {
    // 1. Get CPU model name
    QueryCpuModelName();

    // 2. Get core count
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_coreCount = (int)sysInfo.dwNumberOfProcessors;

    // 3. Initialize buffers
    int historyLen = m_historyLength.load();
    m_coreHistories.resize(m_coreCount);
    for (int i = 0; i < m_coreCount; ++i) {
        m_coreHistories[i].Resize(historyLen);
    }
    m_avgCpuHistory.Resize(historyLen);
    m_memoryHistory.Resize(historyLen);

    // Initialize previous core times buffer
    m_prevCoreTimes.resize(m_coreCount, CpuTimes{});
}

SystemMonitor::~SystemMonitor() {
    Stop();
}

void SystemMonitor::Start() {
    if (m_running) return;

    // Run initial collections to establish baselines
    CollectMetrics();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CollectMetrics();

    m_running = true;
    m_workerThread = std::thread(&SystemMonitor::WorkerLoop, this);
}

void SystemMonitor::Stop() {
    if (!m_running) return;
    m_running = false;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void SystemMonitor::SetIntervalMs(int ms) {
    if (ms < 50) ms = 50; // Limit minimum to 50ms (20 updates/sec) to protect CPU
    m_intervalMs.store(ms);
}

void SystemMonitor::SetHistoryLength(int length) {
    if (length < 10) length = 10;
    if (length > 1000) length = 1000;
    if (length == m_historyLength.load()) return;

    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_historyLength.store(length);

    for (int i = 0; i < m_coreCount; ++i) {
        m_coreHistories[i].Resize(length);
    }
    m_avgCpuHistory.Resize(length);
    m_memoryHistory.Resize(length);
}

void SystemMonitor::GetSnapshot(MonitorSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    snapshot.coreHistories.resize(m_coreCount);
    snapshot.currentCoreUsages.resize(m_coreCount);

    for (int i = 0; i < m_coreCount; ++i) {
        snapshot.coreHistories[i] = m_coreHistories[i].GetContiguous();
        snapshot.currentCoreUsages[i] = m_coreHistories[i].GetLatest();
    }

    snapshot.avgCpuHistory = m_avgCpuHistory.GetContiguous();
    snapshot.memoryHistory = m_memoryHistory.GetContiguous();

    snapshot.currentAvgCpu = m_avgCpuHistory.GetLatest();
    snapshot.currentMemoryPct = m_memoryHistory.GetLatest();

    snapshot.totalMemoryGB = m_totalMemoryGB;
    snapshot.usedMemoryGB = m_usedMemoryGB;
    snapshot.availMemoryGB = m_availMemoryGB;

    if (!snapshot.avgCpuHistory.empty()) {
        snapshot.sampleCount = (int)snapshot.avgCpuHistory.size();
    } else {
        snapshot.sampleCount = 0;
    }
}

void SystemMonitor::WorkerLoop() {
    while (m_running) {
        CollectMetrics();

        // Responsive sleep: sleep in 10ms increments to exit quickly on Stop()
        int sleepRemaining = m_intervalMs.load();
        while (sleepRemaining > 0 && m_running) {
            int sleepTime = (sleepRemaining > 10) ? 10 : sleepRemaining;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            sleepRemaining -= sleepTime;
        }
    }
}

void SystemMonitor::CollectMetrics() {
    // 1. Memory Metrics
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        m_totalMemoryGB = (double)memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        m_availMemoryGB = (double)memInfo.ullAvailPhys / (1024.0 * 1024.0 * 1024.0);
        m_usedMemoryGB = m_totalMemoryGB - m_availMemoryGB;
    }

    // 2. CPU Metrics using NtQuerySystemInformation
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) return;

    PNtQuerySystemInformation NtQuerySystemInformation = 
        (PNtQuerySystemInformation)GetProcAddress(hNtdll, "NtQuerySystemInformation");
    if (!NtQuerySystemInformation) return;

    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> perfInfo(m_coreCount);
    ULONG returnedLength = 0;
    NTSTATUS status = NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        perfInfo.data(),
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * m_coreCount,
        &returnedLength
    );

    if (status != STATUS_SUCCESS) return;

    std::lock_guard<std::mutex> lock(m_dataMutex);

    float totalCpuSum = 0.0f;
    for (int i = 0; i < m_coreCount; ++i) {
        unsigned long long idleTime = perfInfo[i].IdleTime.QuadPart;
        unsigned long long kernelTime = perfInfo[i].KernelTime.QuadPart;
        unsigned long long userTime = perfInfo[i].UserTime.QuadPart;

        // Formula for CPU usage per logical core:
        // KernelTime includes IdleTime in Windows NT structures.
        // Therefore, KernelTime + UserTime = Total cycles of the processor.
        // Active cycles = (KernelTime + UserTime) - IdleTime = UserTime + (KernelTime - IdleTime).
        unsigned long long totalTime = kernelTime + userTime;
        unsigned long long prevTotal = m_prevCoreTimes[i].kernelTime + m_prevCoreTimes[i].userTime;
        unsigned long long prevIdle = m_prevCoreTimes[i].idleTime;

        unsigned long long totalDiff = totalTime - prevTotal;
        unsigned long long idleDiff = idleTime - prevIdle;

        float cpuUsage = 0.0f;
        if (totalDiff > 0) {
            if (totalDiff >= idleDiff) {
                cpuUsage = 100.0f * (1.0f - (float)idleDiff / totalDiff);
            }
        }

        // Clamp to 0.0 - 100.0 to prevent floating point inaccuracies
        if (cpuUsage < 0.0f) cpuUsage = 0.0f;
        if (cpuUsage > 100.0f) cpuUsage = 100.0f;

        m_coreHistories[i].Add(cpuUsage);
        totalCpuSum += cpuUsage;

        m_prevCoreTimes[i].idleTime = idleTime;
        m_prevCoreTimes[i].kernelTime = kernelTime;
        m_prevCoreTimes[i].userTime = userTime;
    }

    float avgCpu = totalCpuSum / m_coreCount;
    m_avgCpuHistory.Add(avgCpu);

    float memUsagePct = 0.0f;
    if (m_totalMemoryGB > 0.0) {
        memUsagePct = (float)((m_usedMemoryGB / m_totalMemoryGB) * 100.0);
    }
    m_memoryHistory.Add(memUsagePct);
}

void SystemMonitor::QueryCpuModelName() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[256] = {0};
        DWORD bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, "ProcessorNameString", nullptr, nullptr, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            m_cpuModelName = buffer;
            
            // Trim leading spaces
            size_t first = m_cpuModelName.find_first_not_of(" \t\r\n");
            if (first != std::string::npos) {
                m_cpuModelName = m_cpuModelName.substr(first);
            }
            
            // Trim trailing spaces
            size_t last = m_cpuModelName.find_last_not_of(" \t\r\n");
            if (last != std::string::npos) {
                m_cpuModelName = m_cpuModelName.substr(0, last + 1);
            }
        }
        RegCloseKey(hKey);
    }

    if (m_cpuModelName.empty()) {
        m_cpuModelName = "Windows Processor";
    }
}
