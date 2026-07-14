#include "system_monitor.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <comdef.h>
#include <Wbemidl.h>
#include "resource_ids.h"
#include <intrin.h>

namespace {
std::wstring GetExeDirectory() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring ws(path);
    size_t pos = ws.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? L"" : ws.substr(0, pos);
}

bool ExtractResource(int resourceId, const std::wstring& outputPath) {
    HRSRC hRes = FindResourceW(NULL, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!hRes) return false;
    
    HGLOBAL hData = LoadResource(NULL, hRes);
    if (!hData) return false;
    
    LPVOID pData = LockResource(hData);
    DWORD size = SizeofResource(NULL, hRes);
    if (!pData || size == 0) return false;
    
    HANDLE hFile = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    
    DWORD written = 0;
    WriteFile(hFile, pData, size, &written, NULL);
    CloseHandle(hFile);
    return true;
}
} // namespace

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
    InitWinRing0();
}

SystemMonitor::~SystemMonitor() {
    Stop();
    ShutdownWinRing0();
    if (m_pdhQuery) {
        PdhCloseQuery((PDH_HQUERY)m_pdhQuery);
        m_pdhQuery = nullptr;
    }
}

void SystemMonitor::InitWinRing0() {
    std::wstring exeDir = GetExeDirectory();
    std::wstring dllPath = exeDir + L"\\WinRing0x64.dll";
    std::wstring sysPath = exeDir + L"\\WinRing0x64.sys";

    // Extract files if missing
    if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        ExtractResource(IDR_WINRING0_DLL, dllPath);
    }
    if (GetFileAttributesW(sysPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        ExtractResource(IDR_WINRING0_SYS, sysPath);
    }

    HMODULE hDll = LoadLibraryW(dllPath.c_str());
    if (hDll) {
        m_hWinRing0 = hDll;
        m_InitializeOls = (InitializeOlsFunc)GetProcAddress(hDll, "InitializeOls");
        m_DeinitializeOls = (DeinitializeOlsFunc)GetProcAddress(hDll, "DeinitializeOls");
        m_GetDllStatus = (GetDllStatusFunc)GetProcAddress(hDll, "GetDllStatus");
        m_ReadPciConfigDwordEx = (ReadPciConfigDwordExFunc)GetProcAddress(hDll, "ReadPciConfigDwordEx");
        m_WritePciConfigDwordEx = (WritePciConfigDwordExFunc)GetProcAddress(hDll, "WritePciConfigDwordEx");
        m_Rdmsr = (RdmsrFunc)GetProcAddress(hDll, "Rdmsr");

        if (m_InitializeOls && m_DeinitializeOls && m_GetDllStatus && 
            m_ReadPciConfigDwordEx && m_WritePciConfigDwordEx && m_Rdmsr) {
            if (m_InitializeOls()) {
                m_winRing0Ready = true;
            }
        }
    }
}

void SystemMonitor::ShutdownWinRing0() {
    if (m_winRing0Ready && m_DeinitializeOls) {
        m_DeinitializeOls();
        m_winRing0Ready = false;
    }
    if (m_hWinRing0) {
        FreeLibrary((HMODULE)m_hWinRing0);
        m_hWinRing0 = nullptr;
    }
}

bool SystemMonitor::IsAmdCpu() {
    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0);
    char vendor[13];
    memcpy(vendor, &cpuInfo[1], 4);     // EBX
    memcpy(vendor + 4, &cpuInfo[3], 4); // EDX
    memcpy(vendor + 8, &cpuInfo[2], 4); // ECX
    vendor[12] = '\0';
    return strcmp(vendor, "AuthenticAMD") == 0;
}

bool SystemMonitor::IsIntelCpu() {
    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0);
    char vendor[13];
    memcpy(vendor, &cpuInfo[1], 4);     // EBX
    memcpy(vendor + 4, &cpuInfo[3], 4); // EDX
    memcpy(vendor + 8, &cpuInfo[2], 4); // ECX
    vendor[12] = '\0';
    return strcmp(vendor, "GenuineIntel") == 0;
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
    snapshot.cpuTemp = m_cpuTemp;
    snapshot.isRealTemp = m_isRealTemp;

    snapshot.totalMemGB = m_totalMemGB;
    snapshot.usedMemGB = m_usedMemGB;
}

void SystemMonitor::WorkerLoop() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool comInitialized = SUCCEEDED(hr);

    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;

    if (comInitialized) {
        // Initialize security (ignore RPC_E_TOO_LATE if already set by another library)
        CoInitializeSecurity(
            NULL, -1, NULL, NULL,
            RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, EOAC_NONE, NULL
        );

        hr = CoCreateInstance(
            CLSID_WbemLocator,
            0,
            CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID *)&pLoc);
        
        if (SUCCEEDED(hr)) {
            hr = pLoc->ConnectServer(
                _bstr_t(L"ROOT\\WMI"),
                NULL, NULL, 0, NULL, 0, 0, &pSvc);
            
            if (SUCCEEDED(hr)) {
                hr = CoSetProxyBlanket(
                   pSvc,
                   RPC_C_AUTHN_WINNT,
                   RPC_C_AUTHZ_NONE,
                   NULL,
                   RPC_C_AUTHN_LEVEL_CALL,
                   RPC_C_IMP_LEVEL_IMPERSONATE,
                   NULL,
                   EOAC_NONE
                );
                if (FAILED(hr)) {
                    pSvc->Release();
                    pSvc = nullptr;
                }
            }
        }
    }

    while (m_running) {
        // Responsive sleep: wake every 50ms so Stop() returns quickly.
        int sleepRemaining = m_intervalMs.load();
        while (sleepRemaining > 0 && m_running) {
            int sleepTime = (sleepRemaining > 50) ? 50 : sleepRemaining;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            sleepRemaining -= sleepTime;
        }
        if (m_running) Collect(pSvc);
    }

    if (pSvc) pSvc->Release();
    if (pLoc) pLoc->Release();
    if (comInitialized) CoUninitialize();
}

void SystemMonitor::Collect(void* pSvc) {
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

    // --- CPU Temperature (WinRing0 -> WMI -> Fallback simulation) ---
    float cpuTemp = 0.0f;
    bool isReal = false;

    // 1. Try WinRing0 direct hardware registers (MSR / SMN)
    if (m_winRing0Ready) {
        bool isAmd = IsAmdCpu();
        bool isIntel = IsIntelCpu();

        if (isAmd && m_WritePciConfigDwordEx && m_ReadPciConfigDwordEx) {
            if (m_WritePciConfigDwordEx(0, 0x60, 0x00059800)) {
                unsigned long val = 0;
                if (m_ReadPciConfigDwordEx(0, 0x64, &val)) {
                    // Formula for Zen: ((val >> 21) & 0x7ff) / 8.0f
                    float temp = ((val >> 21) & 0x7ff) / 8.0f;
                    // On AMD Ryzen, the SMN register reports Tctl, which has a 49.0C offset.
                    // Subtract 49.0C to get Tdie (real core temperature).
                    if (temp > 49.0f) {
                        temp -= 49.0f;
                    }
                    if (temp > 0.0f && temp < 200.0f) {
                        cpuTemp = temp;
                        isReal = true;
                    }
                }
            }
        }
        else if (isIntel && m_Rdmsr) {
            unsigned long eax = 0, edx = 0;
            // Read IA32_THERM_STATUS MSR 0x19C
            if (m_Rdmsr(0x19C, &eax, &edx)) {
                if (eax & 0x80000000) { // Valid bit
                    unsigned long offset = (eax >> 16) & 0x7F;
                    unsigned long tjMax = 100; // default TjMax
                    unsigned long eax2 = 0, edx2 = 0;
                    // Read IA32_TEMPERATURE_TARGET MSR 0x1A2
                    if (m_Rdmsr(0x1A2, &eax2, &edx2)) {
                        unsigned long val = (eax2 >> 16) & 0xFF;
                        if (val > 0) tjMax = val;
                    }
                    float temp = (float)(tjMax - offset);
                    if (temp > 0.0f && temp < 200.0f) {
                        cpuTemp = temp;
                        isReal = true;
                    }
                }
            }
        }
    }

    // 2. Fallback to WMI if WinRing0 was not ready or failed
    if (!isReal && pSvc) {
        IWbemServices* pWmiSvc = static_cast<IWbemServices*>(pSvc);
        IEnumWbemClassObject* pEnumerator = nullptr;
        HRESULT hr = pWmiSvc->ExecQuery(
            _bstr_t(L"WQL"),
            _bstr_t(L"SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator);
        
        if (SUCCEEDED(hr) && pEnumerator) {
            IWbemClassObject* pclsObj = nullptr;
            ULONG uReturn = 0;
            float maxTemp = 0.0f;
            bool gotData = false;
            
            while (pEnumerator) {
                hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (0 == uReturn) break;
                
                VARIANT vtProp;
                hr = pclsObj->Get(L"CurrentTemperature", 0, &vtProp, 0, 0);
                if (SUCCEEDED(hr)) {
                    if (vtProp.vt == VT_I4 || vtProp.vt == VT_UI4) {
                        float tempK_tenths = (float)vtProp.lVal;
                        // (tenths of Kelvin - 2732) / 10 = Celsius
                        float tempC = (tempK_tenths - 2732.0f) / 10.0f;
                        if (tempC > 0.0f && tempC < 200.0f) {
                            if (tempC > maxTemp) {
                                maxTemp = tempC;
                            }
                            gotData = true;
                        }
                    }
                    VariantClear(&vtProp);
                }
                pclsObj->Release();
            }
            pEnumerator->Release();
            if (gotData) {
                cpuTemp = maxTemp;
                isReal = true;
            }
        }
    }

    // 3. Fallback to Load-based simulation if all real methods failed
    if (!isReal) {
        // Fallback simulated temperature based on CPU load (37C idle, up to 82C max)
        cpuTemp = 37.0f + (cpuPct * 0.45f);
    }

    // First call only establishes the delta baselines. Memory readings are
    // instantaneous (not delta-based), so store them right away instead of
    // showing 0/0 GB until the second sample.
    if (!m_hasBaseline) {
        m_hasBaseline = true;
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_totalMemGB = totalGB;
        m_usedMemGB = usedGB;
        m_cpuTemp = cpuTemp;
        m_isRealTemp = isReal;
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
    m_cpuTemp = cpuTemp;
    m_isRealTemp = isReal;
}
