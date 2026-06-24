#include "ui_renderer.h"
#include <imgui.h>
#include <implot.h>
#include <cmath>
#include <cstdio>

static const float PI = 3.1415926535f;

UiRenderer::UiRenderer(SystemMonitor& monitor) : m_monitor(monitor) {
    InitializeColors();
}

UiRenderer::~UiRenderer() {
}

void UiRenderer::ApplyTheme() {
    // 1. Setup ImGui Colors & Styling
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;
    
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.TabBorderSize = 0.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.92f, 0.93f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.48f, 0.50f, 0.55f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.08f, 0.09f, 0.12f, 1.00f); 
    colors[ImGuiCol_ChildBg]                = ImVec4(0.11f, 0.12f, 0.16f, 0.60f); 
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.09f, 0.12f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.18f, 0.20f, 0.26f, 1.00f); 
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.14f, 0.16f, 0.22f, 1.00f); 
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.18f, 0.21f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.23f, 0.26f, 0.36f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.08f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.10f, 0.11f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.08f, 0.09f, 0.12f, 0.50f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.16f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.08f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.24f, 0.27f, 0.36f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.29f, 0.33f, 0.44f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.35f, 0.40f, 0.53f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.26f, 0.59f, 0.98f, 0.90f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.16f, 0.18f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.22f, 0.26f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.28f, 0.33f, 0.45f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.16f, 0.22f, 0.32f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.20f, 0.29f, 0.43f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.38f, 0.56f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.18f, 0.20f, 0.26f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.12f, 0.14f, 0.19f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.20f, 0.24f, 0.33f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.16f, 0.22f, 0.32f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.08f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.12f, 0.14f, 0.19f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.20f, 0.28f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.35f, 0.45f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.25f, 0.32f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

    // 2. Setup ImPlot Theme
    ImPlotStyle& plotStyle = ImPlot::GetStyle();
    
    plotStyle.PlotPadding = ImVec2(10.0f, 10.0f);
    plotStyle.LabelPadding = ImVec2(5.0f, 5.0f);
    plotStyle.LegendPadding = ImVec2(6.0f, 6.0f);
    plotStyle.LegendInnerPadding = ImVec2(4.0f, 4.0f);
    plotStyle.LegendSpacing = ImVec2(4.0f, 4.0f);
    
    plotStyle.PlotMinSize = ImVec2(100.0f, 100.0f);
    
    plotStyle.LineWeight = 2.0f;
    plotStyle.MarkerSize = 4.0f;
    plotStyle.MarkerWeight = 1.0f;
    plotStyle.FillAlpha = 0.15f; 
    
    plotStyle.PlotBorderSize = 1.0f;
    plotStyle.MinorAlpha = 0.1f;
    
    ImVec4* plotColors = plotStyle.Colors;
    plotColors[ImPlotCol_PlotBg]         = ImVec4(0.11f, 0.12f, 0.16f, 0.50f); 
    plotColors[ImPlotCol_PlotBorder]     = ImVec4(0.18f, 0.20f, 0.26f, 0.80f);
    plotColors[ImPlotCol_LegendBg]       = ImVec4(0.08f, 0.09f, 0.12f, 0.85f);
    plotColors[ImPlotCol_LegendBorder]   = ImVec4(0.18f, 0.20f, 0.26f, 0.80f);
    plotColors[ImPlotCol_LegendText]     = ImVec4(0.92f, 0.93f, 0.95f, 1.00f);
    plotColors[ImPlotCol_TitleText]      = ImVec4(0.92f, 0.93f, 0.95f, 1.00f);
    plotColors[ImPlotCol_InlayText]      = ImVec4(0.92f, 0.93f, 0.95f, 1.00f);
    plotColors[ImPlotCol_AxisText]       = ImVec4(0.60f, 0.63f, 0.68f, 1.00f);
    plotColors[ImPlotCol_AxisGrid]       = ImVec4(0.18f, 0.20f, 0.26f, 0.35f); 
    plotColors[ImPlotCol_AxisTick]       = ImVec4(0.18f, 0.20f, 0.26f, 0.60f);
    plotColors[ImPlotCol_Selection]      = ImVec4(0.11f, 0.64f, 0.92f, 0.30f);
    plotColors[ImPlotCol_Crosshairs]     = ImVec4(0.92f, 0.93f, 0.95f, 0.40f);
}

void UiRenderer::Render() {
    // Acquire current snapshot of system stats
    m_monitor.GetSnapshot(m_snapshot);
    
    // Update local variables from monitor settings (in case adjusted elsewhere)
    m_refreshRateMs = m_monitor.GetIntervalMs();
    m_historyLen = m_monitor.GetHistoryLength();

    // Fill screen window setup
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DashboardContainer", nullptr, windowFlags);
    ImGui::PopStyleVar();

    float windowWidth = ImGui::GetWindowWidth();
    float windowHeight = ImGui::GetWindowHeight();

    // Adjust layout for narrow windows
    float sidebarWidth = 260.0f;
    if (windowWidth < 600.0f) {
        sidebarWidth = windowWidth * 0.4f;
    }
    float mainContentWidth = windowWidth - sidebarWidth;

    RenderSidebar(sidebarWidth, windowHeight);
    
    ImGui::SameLine();
    RenderMainContent(sidebarWidth, mainContentWidth, windowHeight);

    ImGui::End();

    // Developer test panels if toggled
    if (m_showImGuiDemo) {
        ImGui::ShowDemoWindow(&m_showImGuiDemo);
    }
    if (m_showImPlotDemo) {
        ImPlot::ShowDemoWindow(&m_showImPlotDemo);
    }
}

void UiRenderer::RenderSidebar(float width, float height) {
    ImGui::BeginChild("Sidebar", ImVec2(width, height), true, ImGuiWindowFlags_NoScrollbar);
    
    // Header
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
    ImGui::Text("  SYS MONITOR SDK");
    ImGui::PopStyleColor();
    
    // Glowing Pulse State Icon
    float pulse = (float)(std::sin(ImGui::GetTime() * 3.5) * 0.5 + 0.5);
    ImGui::Spacing();
    ImGui::Text("  ");
    ImGui::SameLine();
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorScreen = ImGui::GetCursorScreenPos();
    ImVec2 dotCenter = ImVec2(cursorScreen.x + 5.0f, cursorScreen.y + 7.0f);
    
    // Outer glow
    drawList->AddCircleFilled(dotCenter, 6.0f + pulse * 2.0f, IM_COL32(0, 220, 100, (int)(80 * (1.0f - pulse))), 16);
    // Inner dot
    drawList->AddCircleFilled(dotCenter, 4.0f, IM_COL32(0, 220, 100, 255), 16);
    
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 18.0f);
    ImGui::TextColored(ImVec4(0.0f, 0.85f, 0.4f, 1.0f), "Monitoring Live");
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // CPU Stats
    ImGui::TextColored(ImVec4(0.00f, 0.80f, 1.00f, 1.00f), "CPU CORE DETAILS");
    ImGui::Spacing();
    ImGui::TextWrapped("Processor: %s", m_monitor.GetCpuModelName().c_str());
    ImGui::Text("Hardware Threads: %d", m_monitor.GetCoreCount());
    ImGui::Spacing();
    
    ImGui::Text("Overall Utilization:");
    ImGui::TextColored(ImVec4(0.00f, 0.80f, 1.00f, 1.00f), "%.1f %%", m_snapshot.currentAvgCpu);
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.00f, 0.80f, 1.00f, 1.00f));
    ImGui::ProgressBar(m_snapshot.currentAvgCpu / 100.0f, ImVec2(-1, 12), "");
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Memory Stats
    ImGui::TextColored(ImVec4(1.00f, 0.35f, 0.60f, 1.00f), "RAM UTILIZATION");
    ImGui::Spacing();
    ImGui::Text("Total Cap: %.2f GB", m_snapshot.totalMemoryGB);
    ImGui::Text("Used:      %.2f GB", m_snapshot.usedMemoryGB);
    ImGui::Text("Available: %.2f GB", m_snapshot.availMemoryGB);
    ImGui::Spacing();
    
    ImGui::Text("Memory Usage:");
    ImGui::TextColored(ImVec4(1.00f, 0.35f, 0.60f, 1.00f), "%.1f %%", m_snapshot.currentMemoryPct);
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.00f, 0.35f, 0.60f, 1.00f));
    ImGui::ProgressBar(m_snapshot.currentMemoryPct / 100.0f, ImVec2(-1, 12), "");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    
    // Bottom panel layout
    ImGui::SetCursorPosY(height - 35.0f);
    ImGui::TextDisabled("  C++ ImGui DX11 Native App");
    
    ImGui::EndChild();
}

void UiRenderer::RenderMainContent(float xPos, float width, float height) {
    ImGui::BeginChild("MainContent", ImVec2(width, height), false);
    
    int count = m_snapshot.sampleCount;
    std::vector<float> xData(count);
    float dt = m_refreshRateMs / 1000.0f;
    for (int i = 0; i < count; ++i) {
        xData[i] = (float)(i - count + 1) * dt;
    }

    ImGui::Spacing();
    if (ImGui::BeginTabBar("MainTabBar", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Overview")) {
            DrawOverviewTab(dt, xData);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Per-Core Grid")) {
            DrawPerCoreGridTab(dt, xData);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Overlaid Cores")) {
            DrawOverlaidCoresTab(dt, xData);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Memory History")) {
            DrawMemoryTab(dt, xData);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings")) {
            DrawSettingsTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::EndChild();
}

void UiRenderer::DrawOverviewTab(float dt, const std::vector<float>& xData) {
    float availHeight = ImGui::GetContentRegionAvail().y;
    float plotHeight = (availHeight - 35.0f) / 2.0f;
    
    ImGui::Text("Total CPU Core Utilization (Average)");
    if (ImPlot::BeginPlot("##OverviewCpuPlot", ImVec2(-1, plotHeight))) {
        ImPlot::SetupAxes("Time elapsed (s)", "Usage (%)");
        ImPlot::SetupAxisLimits(ImAxis_X1, -m_historyLen * dt, 0.0f, ImPlotCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0f, 105.0f, ImPlotCond_Always);
        
        ImPlot::SetNextLineStyle(ImVec4(0.00f, 0.80f, 1.00f, 1.00f), 2.0f);
        ImPlot::SetNextFillStyle(ImVec4(0.00f, 0.80f, 1.00f, 1.00f), 0.15f);
        
        ImPlot::PlotLine("Average CPU", xData.data(), m_snapshot.avgCpuHistory.data(), m_snapshot.sampleCount);
        ImPlot::PlotShaded("Average CPU", xData.data(), m_snapshot.avgCpuHistory.data(), m_snapshot.sampleCount, 0.0f);
        
        ImPlot::EndPlot();
    }
    
    ImGui::Spacing();
    ImGui::Text("System Physical Memory Usage");
    if (ImPlot::BeginPlot("##OverviewMemoryPlot", ImVec2(-1, plotHeight))) {
        ImPlot::SetupAxes("Time elapsed (s)", "Usage (%)");
        ImPlot::SetupAxisLimits(ImAxis_X1, -m_historyLen * dt, 0.0f, ImPlotCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0f, 105.0f, ImPlotCond_Always);
        
        ImPlot::SetNextLineStyle(ImVec4(1.00f, 0.35f, 0.60f, 1.00f), 2.0f);
        ImPlot::SetNextFillStyle(ImVec4(1.00f, 0.35f, 0.60f, 1.00f), 0.15f);
        
        ImPlot::PlotLine("Memory Load", xData.data(), m_snapshot.memoryHistory.data(), m_snapshot.sampleCount);
        ImPlot::PlotShaded("Memory Load", xData.data(), m_snapshot.memoryHistory.data(), m_snapshot.sampleCount, 0.0f);
        
        ImPlot::EndPlot();
    }
}

void UiRenderer::DrawPerCoreGridTab(float dt, const std::vector<float>& xData) {
    ImGui::BeginChild("CoreGridContainer", ImVec2(0.0f, 0.0f), false);
    
    float availWidth = ImGui::GetContentRegionAvail().x;
    ImGuiStyle& style = ImGui::GetStyle();
    
    // 縮小卡片大小為 1/4 (基準寬度 220f -> 110f，基準高度 145f -> 72.5f)
    int cols = (int)(availWidth / 110.0f);
    if (cols < 1) cols = 1;
    int coreCount = m_monitor.GetCoreCount();
    if (cols > coreCount) cols = coreCount;
    
    float actualCardWidth = (availWidth - style.ItemSpacing.x * (cols - 1) - style.ScrollbarSize) / cols;
    float cardHeight = 72.5f;
    
    // 暫時調整 ImPlot 的 padding，為迷你圖表提供更多顯示空間
    ImPlotStyle& plotStyle = ImPlot::GetStyle();
    ImVec2 oldPlotPadding = plotStyle.PlotPadding;
    plotStyle.PlotPadding = ImVec2(2.0f, 2.0f);
    
    // 暫時調整 ImGui Child padding，讓卡片邊緣更緊湊
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));
    
    for (int i = 0; i < coreCount; ++i) {
        if (i % cols != 0) {
            ImGui::SameLine();
        }
        
        ImGui::PushID(i);
        ImGui::BeginChild("CoreCard", ImVec2(actualCardWidth, cardHeight), true, ImGuiWindowFlags_NoScrollbar);
        
        // 縮小卡片文字寬度與小數點顯示
        ImGui::Text("C%d", i);
        ImGui::SameLine(actualCardWidth - 55.0f);
        ImGui::TextColored(m_coreColors[i], "%.0f%%", m_snapshot.currentCoreUsages[i]);
        
        ImPlotFlags plotFlags = ImPlotFlags_CanvasOnly | ImPlotFlags_NoChild;
        ImPlotAxisFlags xFlags = ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoLabel;
        ImPlotAxisFlags yFlags = ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoLabel;
        
        if (ImPlot::BeginPlot("##CoreLinePlot", ImVec2(-1, -1), plotFlags)) {
            ImPlot::SetupAxes(nullptr, nullptr, xFlags, yFlags);
            ImPlot::SetupAxisLimits(ImAxis_X1, -m_historyLen * dt, 0.0f, ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0f, 105.0f, ImPlotCond_Always);
            
            ImPlot::SetNextLineStyle(m_coreColors[i], 1.2f);
            ImPlot::SetNextFillStyle(m_coreColors[i], 0.10f);
            
            ImPlot::PlotLine("##Line", xData.data(), m_snapshot.coreHistories[i].data(), m_snapshot.sampleCount);
            ImPlot::PlotShaded("##Shade", xData.data(), m_snapshot.coreHistories[i].data(), m_snapshot.sampleCount, 0.0f);
            
            ImPlot::EndPlot();
        }
        
        ImGui::EndChild();
        ImGui::PopID();
    }
    
    ImGui::PopStyleVar(2);
    plotStyle.PlotPadding = oldPlotPadding;
    
    ImGui::EndChild();
}

void UiRenderer::DrawOverlaidCoresTab(float dt, const std::vector<float>& xData) {
    float availHeight = ImGui::GetContentRegionAvail().y;
    
    ImGui::Text("All Core Loads Overlaid (Comparison View)");
    if (ImPlot::BeginPlot("##OverlaidCoresPlot", ImVec2(-1, availHeight - 30.0f))) {
        ImPlot::SetupAxes("Time elapsed (s)", "Utilization (%)");
        ImPlot::SetupAxisLimits(ImAxis_X1, -m_historyLen * dt, 0.0f, ImPlotCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0f, 105.0f, ImPlotCond_Always);
        
        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Outside);
        
        int coreCount = m_monitor.GetCoreCount();
        for (int i = 0; i < coreCount; ++i) {
            char label[32];
            std::sprintf(label, "Core %d (%.0f%%)", i, m_snapshot.currentCoreUsages[i]);
            
            ImPlot::SetNextLineStyle(m_coreColors[i], 1.5f);
            ImPlot::PlotLine(label, xData.data(), m_snapshot.coreHistories[i].data(), m_snapshot.sampleCount);
        }
        
        ImPlot::EndPlot();
    }
}

void UiRenderer::DrawMemoryTab(float dt, const std::vector<float>& xData) {
    float availHeight = ImGui::GetContentRegionAvail().y;
    float plotHeight = availHeight - 35.0f;
    
    ImGui::Columns(2, "MemoryTabSplit", false);
    
    float availWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetColumnWidth(0, availWidth - 200.0f);
    
    ImGui::Text("System RAM Loading Profile");
    if (ImPlot::BeginPlot("##MemoryDetailChart", ImVec2(-1, plotHeight))) {
        ImPlot::SetupAxes("Time elapsed (s)", "Memory Usage (%)");
        ImPlot::SetupAxisLimits(ImAxis_X1, -m_historyLen * dt, 0.0f, ImPlotCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0f, 105.0f, ImPlotCond_Always);
        
        ImPlot::SetNextLineStyle(ImVec4(1.00f, 0.35f, 0.60f, 1.00f), 2.0f);
        ImPlot::SetNextFillStyle(ImVec4(1.00f, 0.35f, 0.60f, 1.00f), 0.15f);
        
        ImPlot::PlotLine("RAM %", xData.data(), m_snapshot.memoryHistory.data(), m_snapshot.sampleCount);
        ImPlot::PlotShaded("RAM %", xData.data(), m_snapshot.memoryHistory.data(), m_snapshot.sampleCount, 0.0f);
        
        ImPlot::EndPlot();
    }
    
    ImGui::NextColumn();
    ImGui::SetColumnWidth(1, 200.0f);
    
    ImGui::Text("Specs & Allocation");
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Text("Total physical: %.2f GB", m_snapshot.totalMemoryGB);
    ImGui::Text("Allocated RAM:  %.2f GB", m_snapshot.usedMemoryGB);
    ImGui::Text("Free RAM:       %.2f GB", m_snapshot.availMemoryGB);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Render Ring Dial
    ImGui::Text("Memory Dial:");
    ImVec2 dialCursor = ImGui::GetCursorScreenPos();
    dialCursor.x += 80.0f;
    dialCursor.y += 60.0f;
    float radius = 55.0f;
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Background track ring
    drawList->AddCircle(dialCursor, radius, IM_COL32(40, 45, 55, 255), 36, 7.0f);
    
    // Active progress arc
    float pct = m_snapshot.currentMemoryPct / 100.0f;
    if (pct > 0.0f) {
        drawList->PathClear();
        drawList->PathArcTo(dialCursor, radius, -PI * 0.5f, -PI * 0.5f + PI * 2.0f * pct, 36);
        drawList->PathStroke(IM_COL32(255, 90, 150, 255), 0, 7.0f);
    }
    
    // Value text in center
    char text[16];
    std::sprintf(text, "%.0f%%", m_snapshot.currentMemoryPct);
    ImVec2 txtSz = ImGui::CalcTextSize(text);
    drawList->AddText(ImVec2(dialCursor.x - txtSz.x / 2.0f, dialCursor.y - txtSz.y / 2.0f), IM_COL32(230, 235, 245, 255), text);
    
    ImGui::Columns(1);
}

void UiRenderer::DrawSettingsTab() {
    ImGui::Text("System Settings");
    ImGui::Separator();
    ImGui::Spacing();
    
    // Poll Speed
    int interval = m_refreshRateMs;
    if (ImGui::SliderInt("Refresh rate (ms)", &interval, 100, 2000)) {
        m_monitor.SetIntervalMs(interval);
    }
    ImGui::TextColored(ImVec4(0.48f, 0.50f, 0.55f, 1.00f), "Controls collection poll frequency (Lower = higher temporal accuracy, higher = lower overhead).");
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Graph duration
    int bufferSz = m_historyLen;
    if (ImGui::SliderInt("Graph point capacity", &bufferSz, 50, 1000)) {
        m_monitor.SetHistoryLength(bufferSz);
    }
    float dt = interval / 1000.0f;
    ImGui::TextColored(ImVec4(0.48f, 0.50f, 0.55f, 1.00f), "Size of the historical ring buffers. Currently displays %.1f seconds of history.", bufferSz * dt);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Debug windows
    ImGui::Text("Diagnostic Utilities");
    ImGui::Checkbox("Display Dear ImGui Demo Window", &m_showImGuiDemo);
    ImGui::Checkbox("Display ImPlot Demo Window", &m_showImPlotDemo);
}

void UiRenderer::InitializeColors() {
    // Beautiful pastel/neon preset palette
    std::vector<ImVec4> basePalette = {
        ImVec4(0.00f, 0.80f, 1.00f, 1.00f), // Neon Cyan-Blue
        ImVec4(0.18f, 0.80f, 0.44f, 1.00f), // Emerald Green
        ImVec4(0.73f, 0.40f, 0.98f, 1.00f), // Electric Violet
        ImVec4(1.00f, 0.35f, 0.60f, 1.00f), // Bright Pink
        ImVec4(1.00f, 0.65f, 0.00f, 1.00f), // Vibrant Orange
        ImVec4(0.00f, 0.98f, 0.60f, 1.00f), // Neon Mint
        ImVec4(1.00f, 0.85f, 0.00f, 1.00f), // Yellow-Gold
        ImVec4(1.00f, 0.27f, 0.00f, 1.00f), // Hot Coral
        ImVec4(0.12f, 0.56f, 1.00f, 1.00f), // Cobalt Blue
        ImVec4(0.60f, 0.98f, 0.00f, 1.00f), // Chartreuse
        ImVec4(0.85f, 0.44f, 0.84f, 1.00f), // Bright Orchid
        ImVec4(0.96f, 0.50f, 0.50f, 1.00f)  // Salmon
    };

    m_coreColors = basePalette;

    int cores = m_monitor.GetCoreCount();
    while (m_coreColors.size() < (size_t)cores) {
        // Golden ratio spacing in HSV space to get unique, harmonious colors
        float hue = (float)(m_coreColors.size() * 0.618033988749895f);
        hue = hue - std::floor(hue); 
        
        float s = 0.80f;
        float v = 0.95f;
        int sector = (int)(hue * 6.0f);
        float frac = hue * 6.0f - sector;
        float p = v * (1.0f - s);
        float q = v * (1.0f - frac * s);
        float t = v * (1.0f - (1.0f - frac) * s);
        
        float r = 0.0f, g = 0.0f, b = 0.0f;
        switch (sector % 6) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }
        m_coreColors.push_back(ImVec4(r, g, b, 1.0f));
    }
}
