#pragma once

#include <windows.h>
#include <shellapi.h>

#include <functional>

namespace npm {

class SystemTray {
public:
    using CommandHandler = std::function<void(UINT commandId)>;

    bool Create(HWND hwnd, HINSTANCE instance, CommandHandler onCommand);
    void Destroy();
    void ShowBalloon(const wchar_t* title, const wchar_t* body);
    bool HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    static constexpr UINT kCallbackMessage = WM_APP + 42;

private:
    HWND hwnd_ = nullptr;
    HINSTANCE instance_ = nullptr;
    NOTIFYICONDATAW nid_{};
    CommandHandler onCommand_;
    bool created_ = false;
};

}  // namespace npm
