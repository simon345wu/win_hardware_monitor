import sys
from imgui_bundle import immapp, hello_imgui
from system_monitor import SystemMonitor
from ui_renderer import UiRenderer

def main():
    # 1. 初始化並啟動背景效能監控線程
    monitor = SystemMonitor()
    monitor.start()

    # 2. 建立 UI 渲染器 (會於第一幀渲染時自動載入 Slate-Dark 配色)
    renderer = UiRenderer(monitor)

    # 3. 定義繪圖 callback
    def gui():
        renderer.render()

    # 4. 配置視窗設定與啟動 ImPlot 附加元件
    runner_params = immapp.RunnerParams()
    runner_params.app_window_params.window_title = "Windows Hardware Performance Monitor (Python)"
    runner_params.app_window_params.window_geometry.size = (1280, 800)
    
    # 關閉預設主視窗，允許我們使用自訂視窗 (DashboardContainer) 填滿視窗
    runner_params.imgui_window_params.default_imgui_window_type = (
        hello_imgui.DefaultImGuiWindowType.no_default_window
    )
    
    # 設定 GUI callback 函數
    runner_params.callbacks.show_gui = gui
    
    addons_params = immapp.AddOnsParams(with_implot=True)

    try:
        # 啟動 Immediate Mode App 循環
        immapp.run(runner_params, addons_params)
    except Exception as e:
        print(f"Application error: {e}", file=sys.stderr)
    finally:
        # 5. 確保程式退出時，背景線程有安全關閉
        monitor.stop()

if __name__ == "__main__":
    main()
