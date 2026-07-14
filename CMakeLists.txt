cmake_minimum_required(VERSION 3.15)
project(WinHardwareMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.8
)
FetchContent_MakeAvailable(imgui)

# Fetch ImPlot
FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG        v0.16
)
FetchContent_MakeAvailable(implot)

# Add our source files
set(SOURCES
    src/main.cpp
    src/system_monitor.h
    src/system_monitor.cpp
    src/ui_renderer.h
    src/ui_renderer.cpp
    src/resources.rc
    
    # ImGui sources (demo translation units excluded to keep the binary small)
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp

    # ImPlot sources
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
)

add_executable(win_hardware_monitor ${SOURCES})

target_include_directories(win_hardware_monitor PRIVATE
    src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${implot_SOURCE_DIR}
)

target_compile_definitions(win_hardware_monitor PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
)

# Link Windows libraries
target_link_libraries(win_hardware_monitor PRIVATE
    d3d11
    dxgi
    d3dcompiler
    dwmapi
    pdh
    iphlpapi
    wbemuuid
)

# Hide console window in Release mode
set_target_properties(win_hardware_monitor PROPERTIES
    WIN32_EXECUTABLE TRUE
)
