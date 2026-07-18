#include "ui_renderer.h"
#include <implot.h>
#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {
const ImVec4 kColorCpu      = ImVec4(0.35f, 0.68f, 0.98f, 1.00f); // blue
const ImVec4 kColorMem      = ImVec4(0.38f, 0.80f, 0.48f, 1.00f); // green
const ImVec4 kColorDiskRead = ImVec4(0.98f, 0.68f, 0.25f, 1.00f); // orange
const ImVec4 kColorDiskWrite= ImVec4(0.94f, 0.40f, 0.40f, 1.00f); // red
const ImVec4 kColorNetDown  = ImVec4(0.68f, 0.52f, 0.96f, 1.00f); // purple
const ImVec4 kColorNetUp    = ImVec4(0.95f, 0.55f, 0.80f, 1.00f); // pink

const ImVec4 kColorTempReal     = ImVec4(0.38f, 0.80f, 0.48f, 1.00f); // green — hardware sensor
const ImVec4 kColorTempFallback = ImVec4(0.98f, 0.68f, 0.25f, 1.00f); // orange — estimated

void FormatRate(float kbPerSec, char* buf, size_t bufSize) {
    if (kbPerSec >= 1024.0f)
        snprintf(buf, bufSize, "%.1fM", kbPerSec / 1024.0f);
    else
        snprintf(buf, bufSize, "%.0fK", kbPerSec);
}

float MaxOf(const std::vector<float>& a, const std::vector<float>& b) {
    float m = 0.0f;
    for (float v : a) if (v > m) m = v;
    for (float v : b) if (v > m) m = v;
    return m;
}
} // namespace

UiRenderer::UiRenderer(SystemMonitor& monitor, void* hwnd)
    : m_monitor(monitor), m_hwnd(hwnd) {
}

void UiRenderer::ApplyTheme(float dpiScale) {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(10.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 3.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.WindowRounding = 0.0f; // OS window has DWM-rounded corners already
    style.PopupRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]          = ImVec4(0.92f, 0.93f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled]  = ImVec4(0.48f, 0.50f, 0.55f, 1.00f);
    colors[ImGuiCol_WindowBg]      = ImVec4(0.08f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg]       = ImVec4(0.10f, 0.11f, 0.15f, 0.98f);
    colors[ImGuiCol_Border]        = ImVec4(0.18f, 0.20f, 0.26f, 1.00f);
    colors[ImGuiCol_Header]        = ImVec4(0.16f, 0.22f, 0.32f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.29f, 0.43f, 1.00f);
    colors[ImGuiCol_HeaderActive]  = ImVec4(0.26f, 0.38f, 0.56f, 1.00f);
    colors[ImGuiCol_Separator]     = ImVec4(0.18f, 0.20f, 0.26f, 1.00f);
    colors[ImGuiCol_CheckMark]     = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

    style.ScaleAllSizes(dpiScale);

    // Crisp text at any DPI; falls back to the built-in bitmap font if missing.
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 15.0f * dpiScale);
    // Smaller face for compact (1/2) mode; 60% keeps labels legible at half size.
    m_fontSmall = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 15.0f * 0.6f * dpiScale);

    ImPlotStyle& plotStyle = ImPlot::GetStyle();
    plotStyle.PlotPadding = ImVec2(0.0f, 1.0f);
    plotStyle.LineWeight = 1.5f;
    plotStyle.FillAlpha = 0.20f;
    plotStyle.PlotBorderSize = 1.0f;
    plotStyle.Colors[ImPlotCol_PlotBg]     = ImVec4(0.11f, 0.12f, 0.16f, 0.55f);
    plotStyle.Colors[ImPlotCol_PlotBorder] = ImVec4(0.18f, 0.20f, 0.26f, 0.80f);
    plotStyle.Colors[ImPlotCol_FrameBg]    = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
}

void UiRenderer::Render() {
    m_monitor.GetSnapshot(m_snapshot);

    // The context menu can flip m_compact mid-frame; push/pop must use the
    // same value or the style/font stacks become unbalanced and assert.
    const bool compact = m_compact;
    if (compact) {
        const ImGuiStyle& s = ImGui::GetStyle();
        ImGui::PushFont(m_fontSmall);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(s.WindowPadding.x * 0.5f, s.WindowPadding.y * 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(s.ItemSpacing.x * 0.5f, s.ItemSpacing.y * 0.5f));
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::Begin("##widget", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // 4 panels split the available height; each panel = header line + plot.
    const ImGuiStyle& style = ImGui::GetStyle();
    float headerH = ImGui::GetTextLineHeight();
    float panelH = (ImGui::GetContentRegionAvail().y - 3.0f * style.ItemSpacing.y) / 4.0f;
    float plotH = panelH - headerH - style.ItemSpacing.y;

    char value[64], a[32], b[32];

    char cpuTempText[32];
    snprintf(cpuTempText, sizeof(cpuTempText), "%.0f°C", m_snapshot.cpuTemp);
    snprintf(value, sizeof(value), "%.0f%%", m_snapshot.cpu);
    DrawPercentPanel("CPU", kColorCpu, m_snapshot.cpuHistory, value, plotH,
                     cpuTempText, m_snapshot.isRealTemp ? kColorTempReal : kColorTempFallback);

    // DIMM temp is only shown when the SPD5118 sensor is readable — no estimate.
    char memTempText[32];
    const char* memTemp = nullptr;
    if (m_snapshot.hasMemTemp) {
        snprintf(memTempText, sizeof(memTempText), "%.0f°C", m_snapshot.memTemp);
        memTemp = memTempText;
    }
    snprintf(value, sizeof(value), "%.0f%%  %.1f/%.1fG",
             m_snapshot.mem, m_snapshot.usedMemGB, m_snapshot.totalMemGB);
    DrawPercentPanel("MEM", kColorMem, m_snapshot.memHistory, value, plotH,
                     memTemp, kColorTempReal);

    FormatRate(m_snapshot.diskRead, a, sizeof(a));
    FormatRate(m_snapshot.diskWrite, b, sizeof(b));
    snprintf(value, sizeof(value), "R %s  W %s", a, b);
    DrawRatePanel("DISK", kColorDiskRead, kColorDiskWrite,
                  m_snapshot.diskReadHistory, m_snapshot.diskWriteHistory, value, plotH);

    FormatRate(m_snapshot.netDown, a, sizeof(a));
    FormatRate(m_snapshot.netUp, b, sizeof(b));
    snprintf(value, sizeof(value), "D %s  U %s", a, b);
    DrawRatePanel("NET", kColorNetDown, kColorNetUp,
                  m_snapshot.netDownHistory, m_snapshot.netUpHistory, value, plotH);

    DrawContextMenu();
    HandleDragging();

    bool popupOpen = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    m_interacting = ImGui::GetIO().WantCaptureMouse || m_dragging || popupOpen;

    ImGui::End();

    if (compact) {
        ImGui::PopStyleVar(2);
        ImGui::PopFont();
    }
}

void UiRenderer::DrawPercentPanel(const char* name, ImVec4 color,
                                  const std::vector<float>& history,
                                  const char* valueText, float plotHeight,
                                  const char* tempText, ImVec4 tempColor) {
    ImGui::TextColored(color, "%s", name);
    float availX = ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX();
    float valueW = ImGui::CalcTextSize(valueText).x;

    if (tempText) {
        float tempW = ImGui::CalcTextSize(tempText).x;
        float spacing = ImGui::GetStyle().ItemSpacing.x * 2.0f;
        ImGui::SameLine(availX - (valueW + spacing + tempW));
        ImGui::TextUnformatted(valueText);
        ImGui::SameLine(availX - tempW);
        ImGui::TextColored(tempColor, "%s", tempText);
    } else {
        ImGui::SameLine(availX - valueW);
        ImGui::TextUnformatted(valueText);
    }

    char plotId[32];
    snprintf(plotId, sizeof(plotId), "##plot_%s", name);
    if (ImPlot::BeginPlot(plotId, ImVec2(-1, plotHeight),
                          ImPlotFlags_CanvasOnly | ImPlotFlags_NoInputs)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, kHistoryLength - 1, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImGuiCond_Always);

        int n = (int)history.size();
        double xStart = (double)(kHistoryLength - n); // newest sample pinned to the right edge
        if (n > 0) {
            ImPlot::SetNextFillStyle(color);
            ImPlot::PlotShaded(plotId, history.data(), n, 0.0, 1.0, xStart);
            ImPlot::SetNextLineStyle(color);
            ImPlot::PlotLine(plotId, history.data(), n, 1.0, xStart);
        }
        ImPlot::EndPlot();
    }
}

void UiRenderer::DrawRatePanel(const char* name, ImVec4 colorA, ImVec4 colorB,
                               const std::vector<float>& historyA,
                               const std::vector<float>& historyB,
                               const char* valueText, float plotHeight) {
    ImGui::TextColored(colorA, "%s", name);
    float valueW = ImGui::CalcTextSize(valueText).x;
    ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - valueW);
    ImGui::TextUnformatted(valueText);

    // A fixed floor keeps idle-level noise from filling the whole chart.
    float yMax = MaxOf(historyA, historyB) * 1.15f;
    if (yMax < 128.0f) yMax = 128.0f; // KB/s

    char plotId[32];
    snprintf(plotId, sizeof(plotId), "##plot_%s", name);
    if (ImPlot::BeginPlot(plotId, ImVec2(-1, plotHeight),
                          ImPlotFlags_CanvasOnly | ImPlotFlags_NoInputs)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, kHistoryLength - 1, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, yMax, ImGuiCond_Always);

        int nA = (int)historyA.size();
        int nB = (int)historyB.size();
        if (nA > 0) {
            ImPlot::SetNextFillStyle(colorA);
            ImPlot::PlotShaded("##a", historyA.data(), nA, 0.0, 1.0, (double)(kHistoryLength - nA));
            ImPlot::SetNextLineStyle(colorA);
            ImPlot::PlotLine("##a", historyA.data(), nA, 1.0, (double)(kHistoryLength - nA));
        }
        if (nB > 0) {
            ImPlot::SetNextLineStyle(colorB);
            ImPlot::PlotLine("##b", historyB.data(), nB, 1.0, (double)(kHistoryLength - nB));
        }
        ImPlot::EndPlot();
    }
}

void UiRenderer::DrawContextMenu() {
    if (ImGui::BeginPopupContextWindow("##ctx", ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem("Always on top", nullptr, &m_topMost)) {
            ApplyTopMost();
        }
        if (ImGui::MenuItem("Compact size (1/2)", nullptr, &m_compact)) {
            ApplyCompactSize();
        }
        ImGui::Separator();
        int interval = m_monitor.GetIntervalMs();
        if (ImGui::MenuItem("0.5s sampling (1 min span)", nullptr, interval == 500))
            m_monitor.SetIntervalMs(500);
        if (ImGui::MenuItem("1s sampling (2 min span)", nullptr, interval == 1000))
            m_monitor.SetIntervalMs(1000);
        if (ImGui::MenuItem("2s sampling (4 min span)", nullptr, interval == 2000))
            m_monitor.SetIntervalMs(2000);
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) {
            m_wantExit = true;
        }
        ImGui::EndPopup();
    }
}

void UiRenderer::HandleDragging() {
    HWND hwnd = (HWND)m_hwnd;
    bool popupOpen = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);

    if (!m_dragging && !popupOpen &&
        ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        POINT cursor;
        RECT rect;
        if (::GetCursorPos(&cursor) && ::GetWindowRect(hwnd, &rect)) {
            m_dragCursorStartX = cursor.x;
            m_dragCursorStartY = cursor.y;
            m_dragWindowStartX = rect.left;
            m_dragWindowStartY = rect.top;
            m_dragging = true;
        }
    }

    if (m_dragging) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            POINT cursor;
            if (::GetCursorPos(&cursor)) {
                ::SetWindowPos(hwnd, nullptr,
                               m_dragWindowStartX + (cursor.x - m_dragCursorStartX),
                               m_dragWindowStartY + (cursor.y - m_dragCursorStartY),
                               0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        } else {
            m_dragging = false;
        }
    }
}

void UiRenderer::ApplyCompactSize() {
    HWND hwnd = (HWND)m_hwnd;
    RECT rect;
    if (!::GetWindowRect(hwnd, &rect))
        return;
    if (m_fullWidth == 0) {
        m_fullWidth = rect.right - rect.left;
        m_fullHeight = rect.bottom - rect.top;
    }
    int w = m_compact ? m_fullWidth / 2 : m_fullWidth;
    int h = m_compact ? m_fullHeight / 2 : m_fullHeight;
    // Keep the top-right corner anchored — the widget docks to the right edge.
    ::SetWindowPos(hwnd, nullptr, rect.right - w, rect.top, w, h,
                   SWP_NOZORDER | SWP_NOACTIVATE);
}

void UiRenderer::ApplyTopMost() {
    ::SetWindowPos((HWND)m_hwnd, m_topMost ? HWND_TOPMOST : HWND_NOTOPMOST,
                   0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}
