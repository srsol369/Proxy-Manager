#pragma once

#include "common/Types.h"
#include "net/TrafficMonitor.h"
#include "tray/SystemTray.h"

#include <windows.h>

#include <string>
#include <vector>

namespace npm {

class Application {
public:
    static Application& Instance();

    bool Initialize(HWND hwnd, HINSTANCE instance);
    void Shutdown();
    void Tick();
    void DrawUi();

    bool WantsExit() const { return wantsExit_; }
    void RequestExit();
    void ShowMainWindow();
    void HideToTray();

    bool HandleTrayMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void OnTrayCommand(UINT commandId);

    void ConnectSelected();
    void Disconnect(bool restorePrevious);
    void PingAll();
    void AddServerFromDraft();
    void RemoveSelected();
    bool ExportServers();
    bool ImportServers();

    std::vector<ServerConfig>& Servers() { return servers_; }
    const ServerConfig* Selected() const;
    ServerConfig* Selected();
    std::vector<std::string> KnownGroups() const;

    int selectedIndex = -1;
    char draftName[128] = "New server";
    char draftHost[256] = "127.0.0.1";
    int draftPort = 8080;
    char draftGroup[64] = {};
    char draftUsername[128] = {};
    char draftPassword[128] = {};
    std::string groupFilter;  // empty = show all groups

    bool connected = false;
    std::string statusLine = "Disconnected";
    std::string lastError;
    TrafficMonitor traffic;
    ProxySnapshot previousOsProxy;
    bool hasPreviousOsProxy = false;

private:
    Application() = default;
    HWND hwnd_ = nullptr;
    SystemTray tray_;
    std::vector<ServerConfig> servers_;
    bool wantsExit_ = false;
};

}  // namespace npm
