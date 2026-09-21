#include "app/Application.h"
#include "ui/Ui.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <d3d11.h>
#include <tchar.h>
#include <dwmapi.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
UINT g_ResizeWidth = 0;
UINT g_ResizeHeight = 0;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

void CreateRenderTarget() {
    ID3D11Texture2D* backBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (backBuffer) {
        g_pd3dDevice->CreateRenderTargetView(backBuffer, nullptr, &g_mainRenderTargetView);
        backBuffer->Release();
    }
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL level;
    const D3D_FEATURE_LEVEL levels[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        levels,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        &level,
        &g_pd3dDeviceContext);
    if (hr == DXGI_ERROR_UNSUPPORTED) {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            levels,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &g_pSwapChain,
            &g_pd3dDevice,
            &level,
            &g_pd3dDeviceContext);
    }
    if (FAILED(hr)) {
        return false;
    }
    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext) {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }

    auto& app = npm::Application::Instance();
    if (app.HandleTrayMessage(msg, wParam, lParam)) {
        return 0;
    }

    switch (msg) {
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED) {
                app.HideToTray();
                return 0;
            }
            g_ResizeWidth = static_cast<UINT>(LOWORD(lParam));
            g_ResizeHeight = static_cast<UINT>(HIWORD(lParam));
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_MINIMIZE) {
                app.HideToTray();
                return 0;
            }
            if ((wParam & 0xfff0) == SC_KEYMENU) {
                return 0;
            }
            break;
        case WM_CLOSE:
            app.HideToTray();
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.lpszClassName = L"NetProxyManagerWindow";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowW(
        wc.lpszClassName,
        L"Net Proxy Manager",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME,
        120,
        120,
        920,
        720,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    npm::Ui::ApplyTheme();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    npm::Application::Instance().Initialize(hwnd, instance);

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) {
                done = true;
            }
        }
        if (done || npm::Application::Instance().WantsExit()) {
            break;
        }

        if (!IsWindowVisible(hwnd)) {
            WaitMessage();
            continue;
        }

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        npm::Application::Instance().Tick();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        npm::Application::Instance().DrawUi();
        ImGui::Render();

        const float clear[4] = {0.07f, 0.08f, 0.10f, 1.0f};
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    npm::Application::Instance().Shutdown();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    CoUninitialize();
    return 0;
}
