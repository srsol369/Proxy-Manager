#include "app/Application.h"

#include "common/FileDialog.h"
#include "config/ConfigStore.h"
#include "ping/Pinger.h"
#include "proxy/WinProxy.h"
#include "ui/Ui.h"

#include "resource.h"

#include <algorithm>

namespace {
constexpr const wchar_t* kSpFilter = L"Proxy List (*.sp)\0*.sp\0All Files\0*.*\0\0";
}  // namespace

namespace npm {

Application& Application::Instance() {
    static Application app;
    return app;
}

bool Application::Initialize(HWND hwnd, HINSTANCE instance) {
    hwnd_ = hwnd;
    servers_ = ConfigStore::Load();
    if (!servers_.empty()) {
        selectedIndex = 0;
    }

    tray_.Create(hwnd, instance, [this](UINT id) { OnTrayCommand(id); });
    statusLine = "Disconnected — proxy is not applied";
    return true;
}

void Application::Shutdown() {
    if (connected) {
        std::string error;
        if (hasPreviousOsProxy) {
            WinProxy::Restore(previousOsProxy, error);
        } else {
            WinProxy::ClearProxy(error);
        }
        connected = false;
    }
    ConfigStore::Save(servers_);
    tray_.Destroy();
}

void Application::Tick() {
    traffic.Tick();
}

void Application::DrawUi() {
    Ui::Draw(*this);
}

void Application::RequestExit() {
    wantsExit_ = true;
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
}

void Application::ShowMainWindow() {
    if (!hwnd_) {
        return;
    }
    ShowWindow(hwnd_, SW_RESTORE);
    SetForegroundWindow(hwnd_);
}

void Application::HideToTray() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
        tray_.ShowBalloon(L"Net Proxy Manager", L"Still running in the notification area.");
    }
}

bool Application::HandleTrayMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    return tray_.HandleMessage(msg, wParam, lParam);
}

void Application::OnTrayCommand(UINT commandId) {
    switch (commandId) {
        case IDM_TRAY_SHOW:
            ShowMainWindow();
            break;
        case IDM_TRAY_CONNECT:
            ConnectSelected();
            break;
        case IDM_TRAY_DISCONNECT:
            Disconnect(true);
            break;
        case IDM_TRAY_EXIT:
            RequestExit();
            break;
        default:
            break;
    }
}

const ServerConfig* Application::Selected() const {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(servers_.size())) {
        return nullptr;
    }
    return &servers_[static_cast<size_t>(selectedIndex)];
}

ServerConfig* Application::Selected() {
    return const_cast<ServerConfig*>(static_cast<const Application*>(this)->Selected());
}

void Application::ConnectSelected() {
    ServerConfig* server = Selected();
    if (!server) {
        lastError = "Select a server first.";
        return;
    }

    std::string error;
    if (!hasPreviousOsProxy) {
        hasPreviousOsProxy = WinProxy::CaptureCurrent(previousOsProxy, error);
    }

    if (!WinProxy::ApplyHttpProxy(server->host, server->port, error)) {
        lastError = error;
        statusLine = "Connect failed";
        connected = false;
        return;
    }

    connected = true;
    lastError.clear();
    statusLine = "Connected — " + server->name + " (" + server->host + ":" + std::to_string(server->port) + ")";
    tray_.ShowBalloon(L"Connected", L"System HTTP proxy was applied.");
}

void Application::Disconnect(bool restorePrevious) {
    std::string error;
    bool ok = false;
    if (restorePrevious && hasPreviousOsProxy) {
        ok = WinProxy::Restore(previousOsProxy, error);
    } else {
        ok = WinProxy::ClearProxy(error);
    }

    connected = false;
    if (!ok) {
        lastError = error;
        statusLine = "Disconnect failed";
        return;
    }
    lastError.clear();
    statusLine = "Disconnected";
}

void Application::PingAll() {
    Pinger::PingAll(servers_);
    if (!servers_.empty()) {
        selectedIndex = 0;
    }
}

void Application::AddServerFromDraft() {
    if (draftHost[0] == '\0' || draftPort <= 0 || draftPort > 65535) {
        lastError = "Host and a valid port are required.";
        return;
    }
    ServerConfig cfg;
    cfg.id = std::to_string(GetTickCount64());
    cfg.name = draftName[0] ? draftName : draftHost;
    cfg.host = draftHost;
    cfg.port = static_cast<uint16_t>(draftPort);
    servers_.push_back(std::move(cfg));
    selectedIndex = static_cast<int>(servers_.size()) - 1;
    ConfigStore::Save(servers_);
    lastError.clear();
}

void Application::RemoveSelected() {
    if (!Selected()) {
        return;
    }
    servers_.erase(servers_.begin() + selectedIndex);
    if (servers_.empty()) {
        selectedIndex = -1;
    } else if (selectedIndex >= static_cast<int>(servers_.size())) {
        selectedIndex = static_cast<int>(servers_.size()) - 1;
    }
    ConfigStore::Save(servers_);
}

void Application::ExportServers() {
    const std::string path = FileDialog::SaveFile(hwnd_, kSpFilter, L"sp", L"proxies.sp");
    if (path.empty()) {
        return;  // user cancelled
    }

    if (ConfigStore::ExportToSp(path, servers_)) {
        lastImportExportMessage = "Exported " + std::to_string(servers_.size()) + " proxies to " + path;
        lastError.clear();
    } else {
        lastImportExportMessage.clear();
        lastError = "Failed to write " + path;
    }
}

void Application::ImportServers() {
    const std::string path = FileDialog::OpenFile(hwnd_, kSpFilter, L"sp");
    if (path.empty()) {
        return;  // user cancelled
    }

    bool ok = false;
    std::vector<ServerConfig> imported = ConfigStore::ImportFromSp(path, ok);
    if (!ok) {
        lastImportExportMessage.clear();
        lastError = "Not a valid .sp file: " + path;
        return;
    }

    int added = 0;
    int updated = 0;
    for (auto& incoming : imported) {
        auto it = std::find_if(servers_.begin(), servers_.end(),
                                [&](const ServerConfig& s) { return s.id == incoming.id; });
        if (it != servers_.end()) {
            *it = incoming;
            ++updated;
        } else {
            servers_.push_back(std::move(incoming));
            ++added;
        }
    }

    if (!servers_.empty() && selectedIndex < 0) {
        selectedIndex = 0;
    }
    ConfigStore::Save(servers_);

    lastImportExportMessage =
        "Imported from " + path + " — added " + std::to_string(added) + ", updated " + std::to_string(updated);
    lastError.clear();
}

}  // namespace npm
