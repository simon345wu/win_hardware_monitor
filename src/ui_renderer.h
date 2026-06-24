#pragma once
#include "system_monitor.h"
#include <vector>
#include <imgui.h>

class UiRenderer {
public:
    UiRenderer(SystemMonitor& monitor);
    ~UiRenderer();

    // Setup style/theme and fonts
    void ApplyTheme();

    // Render current frame
    void Render();

private:
    SystemMonitor& m_monitor;
    MonitorSnapshot m_snapshot;

    // UI state/config
    int m_refreshRateMs = 200;
    int m_historyLen = 300;
    bool m_showImGuiDemo = false;
    bool m_showImPlotDemo = false;
    
    // Core color palette
    std::vector<ImVec4> m_coreColors;
    
    void RenderSidebar(float width, float height);
    void RenderMainContent(float xPos, float width, float height);
    
    void DrawOverviewTab(float dt, const std::vector<float>& xData);
    void DrawPerCoreGridTab(float dt, const std::vector<float>& xData);
    void DrawOverlaidCoresTab(float dt, const std::vector<float>& xData);
    void DrawMemoryTab(float dt, const std::vector<float>& xData);
    void DrawSettingsTab();

    void InitializeColors();
};
