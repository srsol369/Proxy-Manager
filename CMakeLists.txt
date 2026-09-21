cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

project(NetProxyManager VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "NetProxyManager is a Windows-only application (WinINet / Win32 / DirectX 11).")
endif()

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")
if(NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.91.8
        GIT_SHALLOW    TRUE
    )
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()
    set(IMGUI_DIR ${imgui_SOURCE_DIR})
endif()

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_compile_definitions(imgui PUBLIC IMGUI_DEFINE_MATH_OPERATORS)

add_executable(NetProxyManager WIN32
    src/main.cpp
    src/app/Application.cpp
    src/ui/Ui.cpp
    src/proxy/WinProxy.cpp
    src/ping/Pinger.cpp
    src/config/ConfigStore.cpp
    src/common/FileDialog.cpp
    src/tray/SystemTray.cpp
    src/net/TrafficMonitor.cpp
    resources/app.rc
)

target_include_directories(NetProxyManager PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/resources
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    wininet
    # سایر کتابخانه‌های قبلی شما
)
target_compile_definitions(NetProxyManager PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WIN32_WINNT=0x0A00
    NPM_VERSION="${PROJECT_VERSION}"
)

if(MSVC)
    target_compile_options(NetProxyManager PRIVATE /utf-8 /W4 /permissive-)
    target_compile_options(imgui PRIVATE /W3 /utf-8)
    set_property(TARGET NetProxyManager PROPERTY VS_DPI_AWARE "PerMonitor")
else()
    target_compile_options(NetProxyManager PRIVATE -Wall -Wextra)
endif()

target_link_libraries(NetProxyManager PRIVATE
    imgui
    d3d11
    dxgi
    d3dcompiler
    wininet
    winhttp
    iphlpapi
    ws2_32
    shell32
    ole32
    oleaut32
    uuid
    dwmapi
    comctl32
    comdlg32
    user32
    gdi32
    advapi32
)

set_target_properties(NetProxyManager PROPERTIES
    OUTPUT_NAME "NetProxyManager"
    WIN32_EXECUTABLE TRUE
)

if(MINGW)
    target_link_options(NetProxyManager PRIVATE -municode -static-libgcc -static-libstdc++)
endif()
