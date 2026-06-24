from imgui_bundle import imgui, implot
from system_monitor import SystemMonitor, MonitorSnapshot
import numpy as np

class UiRenderer:
    def __init__(self, monitor: SystemMonitor):
        self.monitor = monitor
        self.snapshot = MonitorSnapshot()
        
        self.refresh_rate_ms = 200
        self.history_len = 300
        self.show_imgui_demo = False
        self.show_implot_demo = False
        
        self.core_colors = []
        self._initialize_colors()
        self.theme_applied = False


    def apply_theme(self):
        # 1. Setup ImGui Styling
        style = imgui.get_style()
        
        style.window_padding = imgui.ImVec2(12.0, 12.0)
        style.frame_padding = imgui.ImVec2(6.0, 4.0)
        style.cell_padding = imgui.ImVec2(4.0, 2.0)
        style.item_spacing = imgui.ImVec2(8.0, 8.0)
        style.item_inner_spacing = imgui.ImVec2(6.0, 6.0)
        style.scrollbar_size = 12.0
        style.grab_min_size = 10.0
        
        style.window_rounding = 8.0
        style.child_rounding = 6.0
        style.frame_rounding = 4.0
        style.popup_rounding = 6.0
        style.scrollbar_rounding = 9.0
        style.grab_rounding = 4.0
        style.tab_rounding = 4.0
        
        style.window_border_size = 1.0
        style.child_border_size = 1.0
        style.frame_border_size = 0.0
        style.popup_border_size = 1.0
        style.tab_border_size = 0.0

        # Colors setup
        style.set_color_(imgui.Col_.text, imgui.ImVec4(0.92, 0.93, 0.95, 1.00))
        style.set_color_(imgui.Col_.text_disabled, imgui.ImVec4(0.48, 0.50, 0.55, 1.00))
        style.set_color_(imgui.Col_.window_bg, imgui.ImVec4(0.08, 0.09, 0.12, 1.00))
        style.set_color_(imgui.Col_.child_bg, imgui.ImVec4(0.11, 0.12, 0.16, 0.60))
        style.set_color_(imgui.Col_.popup_bg, imgui.ImVec4(0.08, 0.09, 0.12, 0.98))
        style.set_color_(imgui.Col_.border, imgui.ImVec4(0.18, 0.20, 0.26, 1.00))
        style.set_color_(imgui.Col_.border_shadow, imgui.ImVec4(0.00, 0.00, 0.00, 0.00))
        style.set_color_(imgui.Col_.frame_bg, imgui.ImVec4(0.14, 0.16, 0.22, 1.00))
        style.set_color_(imgui.Col_.frame_bg_hovered, imgui.ImVec4(0.18, 0.21, 0.29, 1.00))
        style.set_color_(imgui.Col_.frame_bg_active, imgui.ImVec4(0.23, 0.26, 0.36, 1.00))
        style.set_color_(imgui.Col_.title_bg, imgui.ImVec4(0.08, 0.09, 0.12, 1.00))
        style.set_color_(imgui.Col_.title_bg_active, imgui.ImVec4(0.10, 0.11, 0.15, 1.00))
        style.set_color_(imgui.Col_.title_bg_collapsed, imgui.ImVec4(0.08, 0.09, 0.12, 0.50))
        style.set_color_(imgui.Col_.menu_bar_bg, imgui.ImVec4(0.14, 0.16, 0.22, 1.00))
        style.set_color_(imgui.Col_.scrollbar_bg, imgui.ImVec4(0.08, 0.09, 0.12, 1.00))
        style.set_color_(imgui.Col_.scrollbar_grab, imgui.ImVec4(0.24, 0.27, 0.36, 1.00))
        style.set_color_(imgui.Col_.scrollbar_grab_hovered, imgui.ImVec4(0.29, 0.33, 0.44, 1.00))
        style.set_color_(imgui.Col_.scrollbar_grab_active, imgui.ImVec4(0.35, 0.40, 0.53, 1.00))
        style.set_color_(imgui.Col_.check_mark, imgui.ImVec4(0.26, 0.59, 0.98, 1.00))
        style.set_color_(imgui.Col_.slider_grab, imgui.ImVec4(0.26, 0.59, 0.98, 0.90))
        style.set_color_(imgui.Col_.slider_grab_active, imgui.ImVec4(0.26, 0.59, 0.98, 1.00))
        style.set_color_(imgui.Col_.button, imgui.ImVec4(0.16, 0.18, 0.25, 1.00))
        style.set_color_(imgui.Col_.button_hovered, imgui.ImVec4(0.22, 0.26, 0.35, 1.00))
        style.set_color_(imgui.Col_.button_active, imgui.ImVec4(0.28, 0.33, 0.45, 1.00))
        style.set_color_(imgui.Col_.header, imgui.ImVec4(0.16, 0.22, 0.32, 1.00))
        style.set_color_(imgui.Col_.header_hovered, imgui.ImVec4(0.20, 0.29, 0.43, 1.00))
        style.set_color_(imgui.Col_.header_active, imgui.ImVec4(0.26, 0.38, 0.56, 1.00))
        style.set_color_(imgui.Col_.separator, imgui.ImVec4(0.18, 0.20, 0.26, 1.00))
        style.set_color_(imgui.Col_.separator_hovered, imgui.ImVec4(0.26, 0.59, 0.98, 0.78))
        style.set_color_(imgui.Col_.separator_active, imgui.ImVec4(0.26, 0.59, 0.98, 1.00))
        style.set_color_(imgui.Col_.resize_grip, imgui.ImVec4(0.26, 0.59, 0.98, 0.20))
        style.set_color_(imgui.Col_.resize_grip_hovered, imgui.ImVec4(0.26, 0.59, 0.98, 0.67))
        style.set_color_(imgui.Col_.resize_grip_active, imgui.ImVec4(0.26, 0.59, 0.98, 0.95))
        style.set_color_(imgui.Col_.tab, imgui.ImVec4(0.12, 0.14, 0.19, 1.00))
        style.set_color_(imgui.Col_.tab_hovered, imgui.ImVec4(0.20, 0.24, 0.33, 1.00))
        style.set_color_(imgui.Col_.tab_selected, imgui.ImVec4(0.16, 0.22, 0.32, 1.00))
        style.set_color_(imgui.Col_.tab_dimmed, imgui.ImVec4(0.08, 0.09, 0.12, 1.00))
        style.set_color_(imgui.Col_.tab_dimmed_selected, imgui.ImVec4(0.12, 0.14, 0.19, 1.00))
        style.set_color_(imgui.Col_.plot_lines, imgui.ImVec4(0.61, 0.61, 0.61, 1.00))
        style.set_color_(imgui.Col_.plot_lines_hovered, imgui.ImVec4(1.00, 0.43, 0.35, 1.00))
        style.set_color_(imgui.Col_.plot_histogram, imgui.ImVec4(0.90, 0.70, 0.00, 1.00))
        style.set_color_(imgui.Col_.plot_histogram_hovered, imgui.ImVec4(1.00, 0.60, 0.00, 1.00))
        style.set_color_(imgui.Col_.table_header_bg, imgui.ImVec4(0.19, 0.20, 0.28, 1.00))
        style.set_color_(imgui.Col_.table_border_strong, imgui.ImVec4(0.31, 0.35, 0.45, 1.00))
        style.set_color_(imgui.Col_.table_border_light, imgui.ImVec4(0.23, 0.25, 0.32, 1.00))
        style.set_color_(imgui.Col_.text_selected_bg, imgui.ImVec4(0.26, 0.59, 0.98, 0.35))

        # 2. Setup ImPlot Styling
        plot_style = implot.get_style()
        plot_style.plot_padding = imgui.ImVec2(10.0, 10.0)
        plot_style.label_padding = imgui.ImVec2(5.0, 5.0)
        plot_style.legend_padding = imgui.ImVec2(6.0, 6.0)
        plot_style.legend_inner_padding = imgui.ImVec2(4.0, 4.0)
        plot_style.legend_spacing = imgui.ImVec2(4.0, 4.0)
        plot_style.plot_min_size = imgui.ImVec2(100.0, 100.0)
        plot_style.plot_border_size = 1.0
        plot_style.minor_alpha = 0.1

        plot_style.set_color_(implot.Col_.plot_bg, imgui.ImVec4(0.11, 0.12, 0.16, 0.50))
        plot_style.set_color_(implot.Col_.plot_border, imgui.ImVec4(0.18, 0.20, 0.26, 0.80))
        plot_style.set_color_(implot.Col_.legend_bg, imgui.ImVec4(0.08, 0.09, 0.12, 0.85))
        plot_style.set_color_(implot.Col_.legend_border, imgui.ImVec4(0.18, 0.20, 0.26, 0.80))
        plot_style.set_color_(implot.Col_.legend_text, imgui.ImVec4(0.92, 0.93, 0.95, 1.00))
        plot_style.set_color_(implot.Col_.title_text, imgui.ImVec4(0.92, 0.93, 0.95, 1.00))
        plot_style.set_color_(implot.Col_.inlay_text, imgui.ImVec4(0.92, 0.93, 0.95, 1.00))
        plot_style.set_color_(implot.Col_.axis_text, imgui.ImVec4(0.60, 0.63, 0.68, 1.00))
        plot_style.set_color_(implot.Col_.axis_grid, imgui.ImVec4(0.18, 0.20, 0.26, 0.35))
        plot_style.set_color_(implot.Col_.axis_tick, imgui.ImVec4(0.18, 0.20, 0.26, 0.60))
        plot_style.set_color_(implot.Col_.selection, imgui.ImVec4(0.11, 0.64, 0.92, 0.30))
        plot_style.set_color_(implot.Col_.crosshairs, imgui.ImVec4(0.92, 0.93, 0.95, 0.40))

    def render(self):
        # 首次渲染 (此時 ImGui/ImPlot Context 已建立) 套用主題
        if not self.theme_applied:
            self.apply_theme()
            self.theme_applied = True

        # 取得背景監控 Snapshot
        self.monitor.get_snapshot(self.snapshot)
        self.refresh_rate_ms = self.monitor.get_interval_ms()
        self.history_len = self.monitor.get_history_length()

        # 設定容器視窗填滿畫面
        viewport = imgui.get_main_viewport()
        imgui.set_next_window_pos(viewport.work_pos)
        imgui.set_next_window_size(viewport.work_size)

        window_flags = (
            imgui.WindowFlags_.no_decoration |
            imgui.WindowFlags_.no_move |
            imgui.WindowFlags_.no_saved_settings |
            imgui.WindowFlags_.no_bring_to_front_on_focus
        )

        imgui.push_style_var(imgui.StyleVar_.window_padding, imgui.ImVec2(0.0, 0.0))
        imgui.begin("DashboardContainer", None, window_flags)
        imgui.pop_style_var()

        window_width = imgui.get_window_width()
        window_height = imgui.get_window_height()

        # 側邊欄寬度響應式調整
        sidebar_width = 260.0
        if window_width < 600.0:
            sidebar_width = window_width * 0.4
        main_content_width = window_width - sidebar_width

        self._render_sidebar(sidebar_width, window_height)
        imgui.same_line()
        self._render_main_content(sidebar_width, main_content_width, window_height)

        imgui.end()

        # 診斷面板
        if self.show_imgui_demo:
            self.show_imgui_demo = imgui.show_demo_window(self.show_imgui_demo)
        if self.show_implot_demo:
            self.show_implot_demo = implot.show_demo_window(self.show_implot_demo)

    def _render_sidebar(self, width, height):
        imgui.begin_child("Sidebar", imgui.ImVec2(width, height), True, imgui.WindowFlags_.no_scrollbar)

        # 標題
        imgui.spacing()
        imgui.push_style_color(imgui.Col_.text, imgui.ImVec4(0.26, 0.59, 0.98, 1.00))
        imgui.text("  SYS MONITOR SDK (PY)")
        imgui.pop_style_color()

        # 呼吸燈動畫
        pulse = float(np.sin(imgui.get_time() * 3.5) * 0.5 + 0.5)
        imgui.spacing()
        imgui.text("  ")
        imgui.same_line()

        draw_list = imgui.get_window_draw_list()
        cursor_screen = imgui.get_cursor_screen_pos()
        dot_center = imgui.ImVec2(cursor_screen.x + 5.0, cursor_screen.y + 7.0)

        color_outer = imgui.get_color_u32(imgui.ImVec4(0.0, 0.86, 0.39, 80 * (1.0 - pulse) / 255.0))
        color_inner = imgui.get_color_u32(imgui.ImVec4(0.0, 0.86, 0.39, 1.0))
        draw_list.add_circle_filled(dot_center, 6.0 + pulse * 2.0, color_outer, 16)
        draw_list.add_circle_filled(dot_center, 4.0, color_inner, 16)

        imgui.set_cursor_pos_x(imgui.get_cursor_pos_x() + 18.0)
        imgui.text_colored(imgui.ImVec4(0.0, 0.85, 0.4, 1.0), "Monitoring Live")

        imgui.spacing()
        imgui.separator()
        imgui.spacing()

        # CPU 基本資訊
        imgui.text_colored(imgui.ImVec4(0.00, 0.80, 1.00, 1.00), "CPU CORE DETAILS")
        imgui.spacing()
        imgui.text_wrapped(f"Processor: {self.monitor.get_cpu_model_name()}")
        imgui.text(f"Hardware Threads: {self.monitor.get_core_count()}")
        imgui.spacing()

        imgui.text("Overall Utilization:")
        imgui.text_colored(imgui.ImVec4(0.00, 0.80, 1.00, 1.00), f"{self.snapshot.current_avg_cpu:.1f} %")

        imgui.push_style_color(imgui.Col_.plot_histogram, imgui.ImVec4(0.00, 0.80, 1.00, 1.00))
        imgui.progress_bar(self.snapshot.current_avg_cpu / 100.0, imgui.ImVec2(-1, 12), "")
        imgui.pop_style_color()

        imgui.spacing()
        imgui.separator()
        imgui.spacing()

        # 記憶體基本資訊
        imgui.text_colored(imgui.ImVec4(1.00, 0.35, 0.60, 1.00), "RAM UTILIZATION")
        imgui.spacing()
        imgui.text(f"Total Cap: {self.snapshot.total_memory_gb:.2f} GB")
        imgui.text(f"Used:      {self.snapshot.used_memory_gb:.2f} GB")
        imgui.text(f"Available: {self.snapshot.avail_memory_gb:.2f} GB")
        imgui.spacing()

        imgui.text("Memory Usage:")
        imgui.text_colored(imgui.ImVec4(1.00, 0.35, 0.60, 1.00), f"{self.snapshot.current_memory_pct:.1f} %")

        imgui.push_style_color(imgui.Col_.plot_histogram, imgui.ImVec4(1.00, 0.35, 0.60, 1.00))
        imgui.progress_bar(self.snapshot.current_memory_pct / 100.0, imgui.ImVec2(-1, 12), "")
        imgui.pop_style_color()

        imgui.spacing()
        imgui.separator()

        # 底部備註
        imgui.set_cursor_pos_y(height - 35.0)
        imgui.text_disabled("  Python ImGui Bundle App")

        imgui.end_child()

    def _render_main_content(self, x_pos, width, height):
        imgui.begin_child("MainContent", imgui.ImVec2(width, height), False)

        count = self.snapshot.sample_count
        dt = self.refresh_rate_ms / 1000.0
        
        # 準備 X 軸時間數據，供 ImPlot 使用
        x_data = np.array([(i - count + 1) * dt for i in range(count)], dtype=np.float32)

        imgui.spacing()
        if imgui.begin_tab_bar("MainTabBar", imgui.TabBarFlags_.none):
            is_selected, _ = imgui.begin_tab_item("Overview")
            if is_selected:
                self._draw_overview_tab(dt, x_data)
                imgui.end_tab_item()
                
            is_selected, _ = imgui.begin_tab_item("Per-Core Grid")
            if is_selected:
                self._draw_per_core_grid_tab(dt, x_data)
                imgui.end_tab_item()
                
            is_selected, _ = imgui.begin_tab_item("Overlaid Cores")
            if is_selected:
                self._draw_overlaid_cores_tab(dt, x_data)
                imgui.end_tab_item()
                
            is_selected, _ = imgui.begin_tab_item("Memory History")
            if is_selected:
                self._draw_memory_tab(dt, x_data)
                imgui.end_tab_item()
                
            is_selected, _ = imgui.begin_tab_item("Settings")
            if is_selected:
                self._draw_settings_tab()
                imgui.end_tab_item()
                
            imgui.end_tab_bar()

        imgui.end_child()

    def _draw_overview_tab(self, dt, x_data):
        avail_height = imgui.get_content_region_avail().y
        plot_height = (avail_height - 35.0) / 2.0

        imgui.text("Total CPU Core Utilization (Average)")
        if implot.begin_plot("##OverviewCpuPlot", imgui.ImVec2(-1, plot_height)):
            implot.setup_axes("Time elapsed (s)", "Usage (%)")
            implot.setup_axis_limits(implot.ImAxis_.x1, -self.history_len * dt, 0.0, imgui.Cond_.always)
            implot.setup_axis_limits(implot.ImAxis_.y1, 0.0, 105.0, imgui.Cond_.always)

            y_data = np.array(self.snapshot.avg_cpu_history, dtype=np.float32)
            if len(y_data) > 0:
                spec = implot.Spec()
                spec.line_color = imgui.ImVec4(0.00, 0.80, 1.00, 1.00)
                spec.line_weight = 2.0
                spec.fill_color = imgui.ImVec4(0.00, 0.80, 1.00, 1.00)
                spec.fill_alpha = 0.15
                implot.plot_line("Average CPU", x_data, y_data, spec)
                implot.plot_shaded("Average CPU", x_data, y_data, 0.0, spec)

            implot.end_plot()

        imgui.spacing()
        imgui.text("System Physical Memory Usage")
        if implot.begin_plot("##OverviewMemoryPlot", imgui.ImVec2(-1, plot_height)):
            implot.setup_axes("Time elapsed (s)", "Usage (%)")
            implot.setup_axis_limits(implot.ImAxis_.x1, -self.history_len * dt, 0.0, imgui.Cond_.always)
            implot.setup_axis_limits(implot.ImAxis_.y1, 0.0, 105.0, imgui.Cond_.always)

            y_data = np.array(self.snapshot.memory_history, dtype=np.float32)
            if len(y_data) > 0:
                spec = implot.Spec()
                spec.line_color = imgui.ImVec4(1.00, 0.35, 0.60, 1.00)
                spec.line_weight = 2.0
                spec.fill_color = imgui.ImVec4(1.00, 0.35, 0.60, 1.00)
                spec.fill_alpha = 0.15
                implot.plot_line("Memory Load", x_data, y_data, spec)
                implot.plot_shaded("Memory Load", x_data, y_data, 0.0, spec)

            implot.end_plot()

    def _draw_per_core_grid_tab(self, dt, x_data):
        imgui.begin_child("CoreGridContainer", imgui.ImVec2(0.0, 0.0), False)

        avail_width = imgui.get_content_region_avail().x
        style = imgui.get_style()

        # 卡片加大 2 倍 (基準寬度 110f -> 220f，高度 72.5f -> 145f)
        cols = int(avail_width / 220.0)
        if cols < 1: cols = 1
        core_count = self.monitor.get_core_count()
        if cols > core_count: cols = core_count

        actual_card_width = (avail_width - style.item_spacing.x * (cols - 1) - style.scrollbar_size) / cols
        card_height = 145.0



        for i in range(core_count):
            if i % cols != 0:
                imgui.same_line()

            imgui.begin_child(f"CoreCard_{i}", imgui.ImVec2(actual_card_width, card_height), True, imgui.WindowFlags_.no_scrollbar)

            imgui.text(f"Core {i}")
            imgui.same_line(actual_card_width - 65.0)
            imgui.text_colored(self.core_colors[i], f"{self.snapshot.current_core_usages[i]:.1f} %")

            plot_flags = implot.Flags_.canvas_only
            x_flags = int(implot.AxisFlags_.no_decorations) | int(implot.AxisFlags_.no_label)
            y_flags = int(implot.AxisFlags_.no_decorations) | int(implot.AxisFlags_.no_label)

            if implot.begin_plot("##CoreLinePlot", imgui.ImVec2(-1, -1), plot_flags):
                implot.setup_axes("", "", x_flags, y_flags)
                implot.setup_axis_limits(implot.ImAxis_.x1, -self.history_len * dt, 0.0, imgui.Cond_.always)
                implot.setup_axis_limits(implot.ImAxis_.y1, 0.0, 105.0, imgui.Cond_.always)

                y_data = np.array(self.snapshot.core_histories[i], dtype=np.float32)
                if len(y_data) > 0:
                    spec = implot.Spec()
                    spec.line_color = self.core_colors[i]
                    spec.line_weight = 1.5
                    spec.fill_color = self.core_colors[i]
                    spec.fill_alpha = 0.12
                    implot.plot_line("##Line", x_data, y_data, spec)
                    implot.plot_shaded("##Shade", x_data, y_data, 0.0, spec)

                implot.end_plot()

            imgui.end_child()



        imgui.end_child()

    def _draw_overlaid_cores_tab(self, dt, x_data):
        avail_height = imgui.get_content_region_avail().y

        imgui.text("All Core Loads Overlaid (Comparison View)")
        if implot.begin_plot("##OverlaidCoresPlot", imgui.ImVec2(-1, avail_height - 30.0)):
            implot.setup_axes("Time elapsed (s)", "Utilization (%)")
            implot.setup_axis_limits(implot.ImAxis_.x1, -self.history_len * dt, 0.0, imgui.Cond_.always)
            implot.setup_axis_limits(implot.ImAxis_.y1, 0.0, 105.0, imgui.Cond_.always)

            implot.setup_legend(implot.Location_.north_west, implot.LegendFlags_.outside)

            core_count = self.monitor.get_core_count()
            for i in range(core_count):
                label = f"Core {i} ({self.snapshot.current_core_usages[i]:.0f}%)"
                y_data = np.array(self.snapshot.core_histories[i], dtype=np.float32)
                if len(y_data) > 0:
                    spec = implot.Spec()
                    spec.line_color = self.core_colors[i]
                    spec.line_weight = 1.5
                    implot.plot_line(label, x_data, y_data, spec)

            implot.end_plot()

    def _draw_memory_tab(self, dt, x_data):
        avail_height = imgui.get_content_region_avail().y
        plot_height = avail_height - 35.0

        imgui.columns(2, "MemoryTabSplit", False)

        avail_width = imgui.get_content_region_avail().x
        imgui.set_column_width(0, avail_width - 200.0)

        imgui.text("System RAM Loading Profile")
        if implot.begin_plot("##MemoryDetailChart", imgui.ImVec2(-1, plot_height)):
            implot.setup_axes("Time elapsed (s)", "Memory Usage (%)")
            implot.setup_axis_limits(implot.ImAxis_.x1, -self.history_len * dt, 0.0, imgui.Cond_.always)
            implot.setup_axis_limits(implot.ImAxis_.y1, 0.0, 105.0, imgui.Cond_.always)

            y_data = np.array(self.snapshot.memory_history, dtype=np.float32)
            if len(y_data) > 0:
                spec = implot.Spec()
                spec.line_color = imgui.ImVec4(1.00, 0.35, 0.60, 1.00)
                spec.line_weight = 2.0
                spec.fill_color = imgui.ImVec4(1.00, 0.35, 0.60, 1.00)
                spec.fill_alpha = 0.15
                implot.plot_line("RAM %", x_data, y_data, spec)
                implot.plot_shaded("RAM %", x_data, y_data, 0.0, spec)

            implot.end_plot()

        imgui.next_column()
        imgui.set_column_width(1, 200.0)

        imgui.text("Specs & Allocation")
        imgui.separator()
        imgui.spacing()
        imgui.text(f"Total physical: {self.snapshot.total_memory_gb:.2f} GB")
        imgui.text(f"Allocated RAM:  {self.snapshot.used_memory_gb:.2f} GB")
        imgui.text(f"Free RAM:       {self.snapshot.avail_memory_gb:.2f} GB")
        imgui.spacing()
        imgui.separator()
        imgui.spacing()

        # 繪製記憶體環形 Dial
        imgui.text("Memory Dial:")
        dial_cursor = imgui.get_cursor_screen_pos()
        dial_cursor.x += 80.0
        dial_cursor.y += 60.0
        radius = 55.0

        draw_list = imgui.get_window_draw_list()
        color_bg = imgui.get_color_u32(imgui.ImVec4(40/255.0, 45/255.0, 55/255.0, 1.0))
        color_arc = imgui.get_color_u32(imgui.ImVec4(1.0, 90/255.0, 150/255.0, 1.0))

        # 背景底圈
        draw_list.add_circle(dial_cursor, radius, color_bg, 36, 7.0)

        # 進度圈弧線
        pct = self.snapshot.current_memory_pct / 100.0
        if pct > 0.0:
            draw_list.path_clear()
            draw_list.path_arc_to(dial_cursor, radius, -np.pi * 0.5, -np.pi * 0.5 + np.pi * 2.0 * pct, 36)
            draw_list.path_stroke(color_arc, 7.0, 0)

        # 中心文字
        text = f"{self.snapshot.current_memory_pct:.0f}%"
        txt_size = imgui.calc_text_size(text)
        txt_pos = imgui.ImVec2(dial_cursor.x - txt_size.x / 2.0, dial_cursor.y - txt_size.y / 2.0)
        
        color_txt = imgui.get_color_u32(imgui.ImVec4(230/255.0, 235/255.0, 245/255.0, 1.0))
        draw_list.add_text(txt_pos, color_txt, text)

        imgui.columns(1)

    def _draw_settings_tab(self):
        imgui.text("System Settings")
        imgui.separator()
        imgui.spacing()

        # 更新頻率調整
        interval = self.refresh_rate_ms
        changed, interval = imgui.slider_int("Refresh rate (ms)", interval, 100, 2000)
        if changed:
            self.monitor.set_interval_ms(interval)
        imgui.text_colored(imgui.ImVec4(0.48, 0.50, 0.55, 1.00), "Controls collection poll frequency (Lower = higher temporal accuracy, higher = lower overhead).")

        imgui.spacing()
        imgui.separator()
        imgui.spacing()

        # 緩衝區容量調整
        buffer_sz = self.history_len
        changed, buffer_sz = imgui.slider_int("Graph point capacity", buffer_sz, 50, 1000)
        if changed:
            self.monitor.set_history_length(buffer_sz)
        dt = interval / 1000.0
        imgui.text_colored(imgui.ImVec4(0.48, 0.50, 0.55, 1.00), f"Size of the historical ring buffers. Currently displays {buffer_sz * dt:.1f} seconds of history.")

        imgui.spacing()
        imgui.separator()
        imgui.spacing()

        # 診斷面板開關
        imgui.text("Diagnostic Utilities")
        _, self.show_imgui_demo = imgui.checkbox("Display Dear ImGui Demo Window", self.show_imgui_demo)
        _, self.show_implot_demo = imgui.checkbox("Display ImPlot Demo Window", self.show_implot_demo)

    def _initialize_colors(self):
        # 內建 12 種配色
        base_palette = [
            imgui.ImVec4(0.00, 0.80, 1.00, 1.00), # Neon Cyan-Blue
            imgui.ImVec4(0.18, 0.80, 0.44, 1.00), # Emerald Green
            imgui.ImVec4(0.73, 0.40, 0.98, 1.00), # Electric Violet
            imgui.ImVec4(1.00, 0.35, 0.60, 1.00), # Bright Pink
            imgui.ImVec4(1.00, 0.65, 0.00, 1.00), # Vibrant Orange
            imgui.ImVec4(0.00, 0.98, 0.60, 1.00), # Neon Mint
            imgui.ImVec4(1.00, 0.85, 0.00, 1.00), # Yellow-Gold
            imgui.ImVec4(1.00, 0.27, 0.00, 1.00), # Hot Coral
            imgui.ImVec4(0.12, 0.56, 1.00, 1.00), # Cobalt Blue
            imgui.ImVec4(0.60, 0.98, 0.00, 1.00), # Chartreuse
            imgui.ImVec4(0.85, 0.44, 0.84, 1.00), # Bright Orchid
            imgui.ImVec4(0.96, 0.50, 0.50, 1.00)  # Salmon
        ]

        self.core_colors = base_palette.copy()
        cores = self.monitor.get_core_count()

        # 超過 12 核心，以黃金比例生成和諧的 HSV 色彩
        while len(self.core_colors) < cores:
            hue = len(self.core_colors) * 0.618033988749895
            hue = hue - np.floor(hue)
            s, v = 0.80, 0.95
            sector = int(hue * 6.0)
            frac = hue * 6.0 - sector
            p = v * (1.0 - s)
            q = v * (1.0 - frac * s)
            t = v * (1.0 - (1.0 - frac) * s)
            
            r, g, b = 0.0, 0.0, 0.0
            sec = sector % 6
            if sec == 0: r, g, b = v, t, p
            elif sec == 1: r, g, b = q, v, p
            elif sec == 2: r, g, b = p, v, t
            elif sec == 3: r, g, b = p, q, v
            elif sec == 4: r, g, b = t, p, v
            elif sec == 5: r, g, b = v, p, q
            
            self.core_colors.append(imgui.ImVec4(r, g, b, 1.0))
