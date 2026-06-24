# Windows Hardware Performance Monitor (WinHardwareMonitor)

一個專為 Windows 平台打造的高性能、即時硬體效能監控工具。本專案使用 **C++17** 開發，並結合 **Dear ImGui** 與 **ImPlot** 打造現代化的 slate-dark 風格操作介面，以極低的系統開銷提供核心級的硬體資訊監控。

---

## 🚀 主要功能

*   **多核心 CPU 即時監控**
    *   自動偵測 CPU 型號與邏輯核心數量。
    *   精準讀取每個核心的即時使用率 (Core Usage)。
*   **全系統記憶體 (RAM) 監控**
    *   提供總記憶體 (Total)、已用記憶體 (Used) 及可用記憶體 (Available) 的即時數據與容量 (GB)。
    *   繪製長週期記憶體使用率變化趨勢圖。
*   **多元的數據可視化面板**
    *   📊 **總覽頁面 (Overview)**：展示 CPU 平均使用率與記憶體使用率的大圖表。
    *   🎛️ **多核網格頁面 (Per-Core Grid)**：將各個邏輯核心的歷史負載分格繪製，方便單獨觀察。
    *   📈 **重疊對比頁面 (Overlaid Cores)**：在同一張圖表中繪製所有核心的波動曲線，利於橫向對比。
    *   💾 **記憶體詳情 (Memory)**：即時分析記憶體使用佔比，結合高精度時序圖。
    *   ⚙️ **系統設定 (Settings)**：可隨時動態調整採樣間隔（最高可達 50ms，即 20Hz 刷新頻率）以及圖表歷史保留長度。
*   **極致的視覺美學**
    *   融入 **Windows 11/10 Immersive Dark Mode** 原生標題列。
    *   採用精心調配的 Slate-Dark 深色主題，搭配平滑的色彩漸變與抗鋸齒圖表。
*   **高性能低開銷**
    *   **核心 API**：捨棄開銷較大的 PDH 或 WMI，改用底層 `NtQuerySystemInformation` API 採樣，確保監控工具本身不佔用顯著的 CPU 資源。
    *   **多執行緒設計**：數據收集於獨立的背景執行緒運行，UI 渲染與時脈解耦。
    *   **記憶體管理**：採用環形緩衝區 (`CircularBuffer`) 儲存歷史數據，避免頻繁的動態記憶體分配。
    *   **GPU 優化**：啟用 VSync 垂直同步，避免 CPU/GPU 無效空轉。

---

## 🛠️ 技術棧 (Tech Stack)

*   **核心語言**：C++17 (MSVC 編譯器)
*   **UI 框架**：[Dear ImGui (v1.90.8)](https://github.com/ocornut/imgui)
*   **圖表套件**：[ImPlot (v0.16)](https://github.com/epezent/implot)
*   **圖形 API**：DirectX 11 + Win32 視窗後端
*   **建置系統**：CMake (3.15+)

---

## 📁 專案結構

```
win_hardware_monitor/
├── CMakeLists.txt         # CMake 設定檔 (自動下載 ImGui & ImPlot 依賴)
├── build.bat              # Windows 一鍵編譯腳本
├── imgui.ini              # ImGui 配置存檔
└── src/
    ├── main.cpp           # 程式入口點、Win32 視窗註冊與 DirectX 11 初始化
    ├── system_monitor.h   # 背景監控執行緒與環形緩衝區宣告
    ├── system_monitor.cpp # 使用 Win32 & NT API 獲取 CPU/記憶體資料的實作
    ├── ui_renderer.h      # UI 渲染類別與面板佈局宣告
    └── ui_renderer.cpp    # Slate-Dark 主題套用與各分頁圖表的渲染實作
```

---

## ⚙️ 環境準備與建置

### 快速一鍵建置 (推薦)

本專案提供了一個自動化建置腳本 [build.bat](file:///c:/antigravityProjects/win_hardware_monitor/build.bat)，它會自動尋找 Windows 上安裝的 **Visual Studio 2022**，載入環境變數，並透過內建的 CMake 編譯出 Release 版本的執行檔。

雙擊執行 `build.bat` 或在終端機中執行：

```powershell
.\build.bat
```

建置成功後，執行檔將產生於：
`build\Release\win_hardware_monitor.exe`

### 手動使用 CMake 建置

如果您偏好手動操作，可以開啟開發者命令提示字元 (Developer Command Prompt for VS 2022) 執行以下步驟：

```bash
# 1. 建立建置目錄
mkdir build
cd build

# 2. 生成建置檔 (預設使用 Visual Studio 2022 x64 產生器)
cmake -G "Visual Studio 17 2022" -A x64 ..

# 3. 進行編譯 (編譯成 Release 模式)
cmake --build . --config Release
```

---

## 💡 技術細節說明

### 1. 核心效能採樣
專案在 [system_monitor.cpp](file:///c:/antigravityProjects/win_hardware_monitor/src/system_monitor.cpp) 中透過動態載入 `ntdll.dll` 並呼叫 `NtQuerySystemInformation` 來獲取邏輯核心的效能資訊：
*   **系統資訊類別**：`SystemProcessorPerformanceInformation` (8)
*   藉由計算兩次採樣之間 `IdleTime`、`KernelTime` 與 `UserTime` 的 Delta 差值，精確計算出每個邏輯核心的使用率。
*   由於 Windows NT 中 `KernelTime` 包含了 `IdleTime`，實際活動時間公式為：
    $$\text{ActiveTime} = (\text{KernelTime} - \text{IdleTime}) + \text{UserTime}$$
    $$\text{CpuUsage\%} = \frac{\text{ActiveTime}}{\text{KernelTime} + \text{UserTime}} \times 100\%$$

### 2. 高效的 CircularBuffer
為避免圖表繪製時因頻繁插入新數據導致 `std::vector` 的重配記憶體與元素移動開銷，專案實作了環形緩衝區 `CircularBuffer`：
*   利用固定容量的 vector 以及頭指針 `head` 記錄最新的數據位置。
*   在需要繪製連續折線圖時，再透過 `GetContiguous()` 以一次性的記憶體複製產出連續的陣列，提供給 ImPlot 進行極速渲染。

---

## ⚖️ 授權協議

本專案基於 **MIT License** 開源。請參閱專案相關說明以獲得更多資訊。
