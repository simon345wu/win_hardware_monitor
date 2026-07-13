#include "system_monitor.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <pdh.h>
#include <pdhmsg.h>

SystemMonitor::SystemMonitor() {
    // Disk throughput comes from PDH. PdhAddEnglishCounter keeps the counter
    // path working on localized (non-English) Windows installs.
    PDH_HQUERY query = nullptr;
    if (PdhOpenQueryW(nullptr, 0, &query) == ERROR_SUCCESS) {
        m_pdhQuery = query;
        PDH_HCOUNTER counter = nullptr;
        if (PdhAddEnglishCounterW(query, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &counter) == ERROR_SUCCESS)
            m_diskReadCounter = counter;
        if (PdhAddEnglishCounterW(query, L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &counter) == ERROR_SUCCESS)
            m_diskWriteCounter = counter;
        PdhCollectQueryData(query); // baseline for rate counters
    }
}

SystemMonitor::~SystemMonitor() {
    Stop();
    if (m_pdhQuery) {
        PdhCloseQuery((PDH_HQUERY)m_pdhQuery);
        m_pdhQuery = nullptr;
    }
}

void SystemMonitor::Start() {
    if (m_running) return;
    Collect(); // establish deltas baseline; first call records nothing
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
    if (ms < 250) ms = 250;
    if (ms > 5000) ms = 5000;
    m_intervalMs.store(ms);
}

void SystemMonitor::GetSnapshot(MonitorSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    snapshot.cpuHistory = m_cpuHistory.GetContiguous();
    snapshot.memHistory = m_memHistory.GetContiguous();
    snapshot.diskReadHistory = m_diskReadHistory.GetContiguous();
    snapshot.diskWriteHistory = m_diskWriteHistory.GetContiguous();
    snapshot.netDownHistory = m_netDownHistory.GetContiguous();
    snapshot.netUpHistory = m_netUpHistory.GetContiguous();

    snapshot.cpu = m_cpuHistory.GetLatest();
    snapshot.mem = m_memHistory.GetLatest();
    snapshot.diskRead = m_diskReadHistory.GetLatest();
    snapshot.diskWrite = m_diskWriteHistory.GetLatest();
    snapshot.netDown = m_netDownHistory.GetLatest();
    snapshot.netUp = m_netUpHistory.GetLatest();

    snapshot.totalMemGB = m_totalMemGB;
    snapshot.usedMemGB = m_usedMemGB;
}

void SystemMonitor::WorkerLoop() {
    while (m_running) {
        // Responsive sleep: wake every 50ms so Stop() returns quickly.
        int sleepRemaining = m_intervalMs.load();
        while (sleepRemaining > 0 && m_running) {
            int sleepTime = (sleepRemaining > 50) ? 50 : sleepRemaining;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            sleepRemaining -= sleepTime;
        }
        if (m_running) Collect();
    }
}

void SystemMonitor::Collect() {
    using namespace std::chrono;
    auto now = steady_clock::now();
    double elapsed = m_hasBaseline ? duration<double>(now - m_prevSampleTime).count() : 0.0;
    m_prevSampleTime = now;

    // --- CPU (system-wide, single call) ---
    float cpuPct = 0.0f;
    FILETIME idleFt, kernelFt, userFt;
    if (GetSystemTimes(&idleFt, &kernelFt, &userFt)) {
        auto toU64 = [](const FILETIME& ft) {
            return ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
        };
        unsigned long long idle = toU64(idleFt);
        unsigned long long kernel = toU64(kernelFt); // includes idle time
        unsigned long long user = toU64(userFt);

        unsigned long long totalDiff = (kernel - m_prevKernel) + (user - m_prevUser);
        unsigned long long idleDiff = idle - m_prevIdle;
        if (m_hasBaseline && totalDiff > 0 && totalDiff >= idleDiff) {
            cpuPct = 100.0f * (1.0f - (float)((double)idleDiff / (double)totalDiff));
        }
        m_prevIdle = idle;
        m_prevKernel = kernel;
        m_prevUser = user;
    }
    if (cpuPct < 0.0f) cpuPct = 0.0f;
    if (cpuPct > 100.0f) cpuPct = 100.0f;

    // --- Memory ---
    MEMORYSTATUSEX memInfo{};
    memInfo.dwLength = sizeof(memInfo);
    double totalGB = 0.0, usedGB = 0.0;
    float memPct = 0.0f;
    if (GlobalMemoryStatusEx(&memInfo)) {
        const double GB = 1024.0 * 1024.0 * 1024.0;
        totalGB = (double)memInfo.ullTotalPhys / GB;
        usedGB = totalGB - (double)memInfo.ullAvailPhys / GB;
        if (totalGB > 0.0) memPct = (float)(usedGB / totalGB * 100.0);
    }

    // --- Disk read/write (PDH, bytes/sec formatted by PDH itself) ---
    float diskReadKB = 0.0f, diskWriteKB = 0.0f;
    if (m_pdhQuery && PdhCollectQueryData((PDH_HQUERY)m_pdhQuery) == ERROR_SUCCESS) {
        PDH_FMT_COUNTERVALUE value{};
        if (m_diskReadCounter &&
            PdhGetFormattedCounterValue((PDH_HCOUNTER)m_diskReadCounter, PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS &&
            (value.CStatus == PDH_CSTATUS_VALID_DATA || value.CStatus == PDH_CSTATUS_NEW_DATA)) {
            diskReadKB = (float)(value.doubleValue / 1024.0);
        }
        if (m_diskWriteCounter &&
            PdhGetFormattedCounterValue((PDH_HCOUNTER)m_diskWriteCounter, PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS &&
            (value.CStatus == PDH_CSTATUS_VALID_DATA || value.CStatus == PDH_CSTATUS_NEW_DATA)) {
            diskWriteKB = (float)(value.doubleValue / 1024.0);
        }
    }

    // --- Network (sum of connected physical adapters, delta of octet counters) ---
    float netDownKB = 0.0f, netUpKB = 0.0f;
    MIB_IF_TABLE2* ifTable = nullptr;
    if (GetIfTable2(&ifTable) == NO_ERROR && ifTable) {
        unsigned long long inOctets = 0, outOctets = 0;
        for (ULONG i = 0; i < ifTable->NumEntries; ++i) {
            const MIB_IF_ROW2& row = ifTable->Table[i];
            // Only physical, connected adapters — skips loopback and virtual
            // adapters that would double-count the same traffic.
            if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK) continue;
            if (!row.InterfaceAndOperStatusFlags.HardwareInterface) continue;
            if (row.MediaConnectState != MediaConnectStateConnected) continue;
            inOctets += row.InOctets;
            outOctets += row.OutOctets;
        }
        FreeMibTable(ifTable);

        if (m_hasBaseline && elapsed > 0.0) {
            if (inOctets >= m_prevNetIn)
                netDownKB = (float)((double)(inOctets - m_prevNetIn) / elapsed / 1024.0);
            if (outOctets >= m_prevNetOut)
                netUpKB = (float)((double)(outOctets - m_prevNetOut) / elapsed / 1024.0);
        }
        m_prevNetIn = inOctets;
        m_prevNetOut = outOctets;
    }

    // First call only establishes the delta baselines. Memory readings are
    // instantaneous (not delta-based), so store them right away instead of
    // showing 0/0 GB until the second sample.
    if (!m_hasBaseline) {
        m_hasBaseline = true;
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_totalMemGB = totalGB;
        m_usedMemGB = usedGB;
        return;
    }

    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_cpuHistory.Add(cpuPct);
    m_memHistory.Add(memPct);
    m_diskReadHistory.Add(diskReadKB);
    m_diskWriteHistory.Add(diskWriteKB);
    m_netDownHistory.Add(netDownKB);
    m_netUpHistory.Add(netUpKB);
    m_totalMemGB = totalGB;
    m_usedMemGB = usedGB;
}
