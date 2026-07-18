#pragma once
#include "system_monitor.h"
#include <imgui.h>

class UiRenderer {
public:
    // hwnd is the top-level Win32 window (void* keeps windows.h out of this header).
    UiRenderer(SystemMonitor& monitor, void* hwnd);

    void ApplyTheme(float dpiScale);
    void Render();

    bool WantExit() const { return m_wantExit; }
    // True while the user hovers or drags the widget — main loop raises the
    // frame rate during interaction and idles otherwise.
    bool IsInteracting() const { return m_interacting; }

private:
    // tempText (optional) renders right-aligned after valueText in tempColor.
    void DrawPercentPanel(const char* name, ImVec4 color, const std::vector<float>& history,
                          const char* valueText, float plotHeight,
                          const char* tempText = nullptr, ImVec4 tempColor = ImVec4());
    void DrawRatePanel(const char* name, ImVec4 colorA, ImVec4 colorB,
                       const std::vector<float>& historyA, const std::vector<float>& historyB,
                       const char* valueText, float plotHeight);
    void DrawContextMenu();
    void HandleDragging();
    void ApplyTopMost();
    void ApplyCompactSize();

    SystemMonitor& m_monitor;
    void* m_hwnd;
    MonitorSnapshot m_snapshot;

    bool m_wantExit = false;
    bool m_topMost = true;
    bool m_interacting = false;

    bool m_compact = false;
    // Full window size in physical pixels, captured on the first compact toggle.
    int m_fullWidth = 0, m_fullHeight = 0;
    ImFont* m_fontSmall = nullptr;

    bool m_dragging = false;
    int m_dragCursorStartX = 0, m_dragCursorStartY = 0;
    int m_dragWindowStartX = 0, m_dragWindowStartY = 0;
};
