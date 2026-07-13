# Windows Resource Monitor Widget

一個常駐桌面角落的輕量級 Windows 即時資源監控小工具。目的很單純:**一眼看出電腦現在有沒有在工作**。

使用 **C++17** + **Dear ImGui** + **ImPlot** + **DirectX 11** 開發,啟動後停靠在螢幕右上角,以四條折線圖顯示最近一段時間的系統活動。

<p align="center"><img src="docs/screenshot.png" width="360" alt="Widget screenshot"></p>

---

## 🚀 功能

*   **四項核心指標**,各一條時間軸折線圖(新資料由右側推進,預設保留 2 分鐘):
    *   **CPU**(藍)— 全系統平均使用率 (%)
    *   **MEM**(綠)— 記憶體使用率 (%),右上顯示 已用/總量 GB(與工作管理員口徑一致)
    *   **DISK** — 磁碟讀取 R(橘)/ 寫入 W(紅)速率,自動以 KB/s、MB/s 顯示
    *   **NET** — 網路下載 D(紫)/ 上傳 U(粉)速率
*   **小巧不擾人**
    *   無邊框小視窗(300×296 邏輯像素),啟動時自動停靠工作區右上角
    *   永遠置頂(可關閉)、不佔工作列、不出現在 Alt-Tab
    *   Windows 11 圓角、完整支援高 DPI 縮放(字體與版面隨系統縮放,實測 225% 清晰)
*   **極低系統開銷**(於 32 核心機器實測)
    *   閒置 CPU:約單核心 0.8%(整機 < 0.1%)
    *   記憶體工作集:約 46 MB;執行檔:約 470 KB,單一檔案免安裝

## 🖱️ 操作

| 操作 | 效果 |
|------|------|
| 左鍵拖曳 | 移動視窗到任意位置 |
| 右鍵 | 選單:置頂開關、採樣間隔(0.5s / 1s / 2s,對應 1 / 2 / 4 分鐘視窗)、結束 |

### 開機自動啟動(選用)

按 `Win+R` 輸入 `shell:startup`,在開啟的資料夾中為 `build\Release\win_hardware_monitor.exe` 建立捷徑即可。

---

## ⚙️ 建置

需求:Visual Studio 2022(含 C++ 桌面開發工作負載)。執行一鍵建置腳本:

```powershell
.\build.bat
```

或手動使用 CMake:

```powershell
cmake -G "Visual Studio 17 2022" -A x64 -B build
cmake --build build --config Release
```

執行檔輸出於 `build\Release\win_hardware_monitor.exe`。首次設定會由 CMake FetchContent 自動下載 ImGui 與 ImPlot,需要網路連線。

### 疑難排解

*   **CMake 報 cache 目錄不符**:專案資料夾搬移過位置時,舊的 `build/` 快取會記著原路徑。刪除整個 `build/` 資料夾重新建置即可。
*   **視窗看不到**:請勿為此視窗加上 `WS_EX_LAYERED`(分層視窗)——DXGI swap chain 無法合成到分層視窗上,視窗會完全不顯示(詳見 `src/main.cpp` 註解)。

---

## 🛠️ 技術說明

### 技術棧

*   C++17 (MSVC)、CMake 3.15+(FetchContent 管理依賴)
*   [Dear ImGui v1.90.8](https://github.com/ocornut/imgui) + [ImPlot v0.16](https://github.com/epezent/implot)
*   DirectX 11 + Win32 後端

### 數據來源

| 指標 | API | 說明 |
|------|-----|------|
| CPU | `GetSystemTimes` | 以 idle/kernel/user 時間的兩次採樣差值計算;單次呼叫,不逐核採樣 |
| 記憶體 | `GlobalMemoryStatusEx` | 已用 = 總實體記憶體 − 可用;與工作管理員「使用中」相同口徑 |
| 磁碟 | PDH `\PhysicalDisk(_Total)\Disk Read/Write Bytes/sec` | 以 `PdhAddEnglishCounter` 加入計數器,中文等本地化 Windows 也能運作 |
| 網路 | `GetIfTable2` | 加總「實體且已連線」介面卡的 octet 計數差值;排除 loopback 與虛擬介面卡,避免同一流量重複計算 |

### 低開銷設計

*   **採樣執行緒**:獨立背景執行緒,預設 1 秒採樣一次;以 50ms 為單位的回應式 sleep,結束程式時能即時退出。
*   **閒置節流渲染**:主迴圈以 `MsgWaitForMultipleObjectsEx` 等待,無互動時約 2 FPS;任何輸入事件立即喚醒並提速至 60 FPS,拖曳不卡頓。
*   **環形緩衝區**:每項指標固定保留 120 筆歷史,無反覆動態記憶體配置;UI 執行緒透過互斥鎖取得快照,所有指標寫入皆在鎖內(執行緒安全)。
*   **編譯瘦身**:不編譯 ImGui/ImPlot demo 原始碼;版面固定,不產生 imgui.ini。

---

## 📁 專案結構

```
win_hardware_monitor/
├── CMakeLists.txt         # CMake 設定(FetchContent 下載 ImGui & ImPlot)
├── build.bat              # 一鍵編譯腳本(自動尋找 VS 2022)
├── docs/screenshot.png    # 執行畫面
├── src/
│   ├── main.cpp           # Win32 無邊框視窗、D3D11 初始化、閒置節流主迴圈
│   ├── system_monitor.h   # 環形緩衝區與監控執行緒宣告
│   ├── system_monitor.cpp # CPU / 記憶體 / 磁碟 / 網路採樣實作
│   ├── ui_renderer.h      # 小工具面板宣告
│   └── ui_renderer.cpp    # 四格折線圖、右鍵選單、拖曳移動、主題
└── py_monitor/            # 舊版多分頁監控的 Python 實作(未跟進改版,僅供參考)
```

## 📝 版本沿革

*   **v2(目前)**— 改寫為桌面角落小工具:僅保留 CPU / MEM / DISK / NET 四指標折線圖,新增磁碟與網路監控,大幅降低資源佔用。
*   **v1** — 多分頁完整監控介面(逐核 CPU 網格、重疊曲線、記憶體詳情、設定頁)。如需舊版可查 git 歷史第一個 commit,或參考 `py_monitor/` 的 Python 實作。

---

## ⚖️ 授權協議

本專案基於 **MIT License** 開源。
