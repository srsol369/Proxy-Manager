#include "tray/SystemTray.h"

#include "resource.h"

#include <shellapi.h>

namespace npm {

bool SystemTray::Create(HWND hwnd, HINSTANCE instance, CommandHandler onCommand) {
    hwnd_ = hwnd;
    instance_ = instance;
    onCommand_ = std::move(onCommand);

    nid_ = {};
    nid_.cbSize = sizeof(nid_);
    nid_.hWnd = hwnd;
    nid_.uID = 1;
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid_.uCallbackMessage = kCallbackMessage;
    nid_.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcsncpy(nid_.szTip, L"Net Proxy Manager", ARRAYSIZE(nid_.szTip) - 1);
    nid_.szTip[ARRAYSIZE(nid_.szTip) - 1] = L'\0';

    created_ = Shell_NotifyIconW(NIM_ADD, &nid_) == TRUE;
    if (created_) {
        nid_.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &nid_);
    }
    return created_;
}

void SystemTray::Destroy() {
    if (created_) {
        Shell_NotifyIconW(NIM_DELETE, &nid_);
        created_ = false;
    }
}

void SystemTray::ShowBalloon(const wchar_t* title, const wchar_t* body) {
    if (!created_) {
        return;
    }
    nid_.uFlags = NIF_INFO | NIF_ICON | NIF_TIP | NIF_MESSAGE;
    wcsncpy(nid_.szInfoTitle, title, ARRAYSIZE(nid_.szInfoTitle) - 1);
    nid_.szInfoTitle[ARRAYSIZE(nid_.szInfoTitle) - 1] = L'\0';
    wcsncpy(nid_.szInfo, body, ARRAYSIZE(nid_.szInfo) - 1);
    nid_.szInfo[ARRAYSIZE(nid_.szInfo) - 1] = L'\0';
    nid_.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

bool SystemTray::HandleMessage(UINT msg, WPARAM /*wParam*/, LPARAM lParam) {
    if (msg != kCallbackMessage) {
        return false;
    }

    const UINT event = LOWORD(lParam);
    if (event == WM_LBUTTONDBLCLK || event == NIN_SELECT || event == NIN_KEYSELECT) {
        if (onCommand_) {
            onCommand_(IDM_TRAY_SHOW);
        }
        return true;
    }

    if (event == WM_RBUTTONUP || event == WM_CONTEXTMENU) {
        POINT pt{};
        GetCursorPos(&pt);
        HMENU menu = LoadMenuW(instance_, MAKEINTRESOURCEW(IDR_TRAYMENU));
        HMENU popup = menu ? GetSubMenu(menu, 0) : nullptr;
        HMENU owned = nullptr;
        if (!popup) {
            owned = CreatePopupMenu();
            AppendMenuW(owned, MF_STRING, IDM_TRAY_SHOW, L"Show window");
            AppendMenuW(owned, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(owned, MF_STRING, IDM_TRAY_CONNECT, L"Connect");
            AppendMenuW(owned, MF_STRING, IDM_TRAY_DISCONNECT, L"Disconnect");
            AppendMenuW(owned, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(owned, MF_STRING, IDM_TRAY_EXIT, L"Exit");
            popup = owned;
        }
        SetForegroundWindow(hwnd_);
        const UINT cmd = TrackPopupMenu(
            popup,
            TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
            pt.x,
            pt.y,
            0,
            hwnd_,
            nullptr);
        if (menu) {
            DestroyMenu(menu);
        }
        if (owned) {
            DestroyMenu(owned);
        }
        if (cmd && onCommand_) {
            onCommand_(static_cast<UINT>(cmd));
        }
        return true;
    }
    return true;
}

}  // namespace npm
