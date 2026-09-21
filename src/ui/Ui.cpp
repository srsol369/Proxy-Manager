#include "ui/Ui.h"

#include "app/Application.h"

#include "imgui.h"

#ifndef NPM_VERSION
#define NPM_VERSION "0.1.0"
#endif

namespace npm {
namespace {

void HelpMarker(const char* text) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

}  // namespace

void Ui::ApplyTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.WindowPadding = ImVec2(16, 16);
    style.FramePadding = ImVec2(10, 6);
    style.ItemSpacing = ImVec2(10, 8);
    style.ScrollbarRounding = 8.0f;

    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.0f);
    c[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.12f, 0.15f, 1.0f);
    c[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.17f, 0.21f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.20f, 0.45f, 0.85f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.52f, 0.92f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.38f, 0.75f, 1.0f);
    c[ImGuiCol_Header] = ImVec4(0.18f, 0.32f, 0.55f, 1.0f);
    c[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.80f, 1.00f, 1.0f);
}

void Ui::Draw(Application& app) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_MenuBar;

    ImGui::Begin("Net Proxy Manager", nullptr, flags);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import server list...")) {
                app.ImportServers();
            }
            if (ImGui::MenuItem("Export server list...")) {
                app.ExportServers();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Minimize to tray")) {
                app.HideToTray();
            }
            if (ImGui::MenuItem("Exit")) {
                app.RequestExit();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::TextUnformatted("Net Proxy Manager");
    ImGui::SameLine();
    ImGui::TextDisabled("MVP  " NPM_VERSION);
    ImGui::Separator();

    // Group filter
    const auto groups = app.KnownGroups();
    ImGui::SetNextItemWidth(220);
    const std::string currentFilterLabel = app.groupFilter.empty() ? "All groups" : app.groupFilter;
    if (ImGui::BeginCombo("Group filter", currentFilterLabel.c_str())) {
        if (ImGui::Selectable("All groups", app.groupFilter.empty())) {
            app.groupFilter.clear();
        }
        for (const auto& g : groups) {
            if (ImGui::Selectable(g.c_str(), app.groupFilter == g)) {
                app.groupFilter = g;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::BeginChild("servers", ImVec2(0, 260), ImGuiChildFlags_Borders);
    if (ImGui::BeginTable("server_table", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Host", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Port", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Delay", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < static_cast<int>(app.Servers().size()); ++i) {
            auto& s = app.Servers()[static_cast<size_t>(i)];
            if (!app.groupFilter.empty() && s.group != app.groupFilter) {
                continue;
            }
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const bool selected = (app.selectedIndex == i);
            if (ImGui::Selectable(s.name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
                app.selectedIndex = i;
            }
            ImGui::TableNextColumn();
            if (s.group.empty()) {
                ImGui::TextDisabled("—");
            } else {
                ImGui::TextUnformatted(s.group.c_str());
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(s.host.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%u", s.port);
            ImGui::TableNextColumn();
            if (s.lastDelayMs) {
                ImGui::Text("%d ms", *s.lastDelayMs);
            } else {
                ImGui::TextDisabled("—");
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(s.lastPingOk ? "OK" : "n/a");
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::Spacing();
    if (ImGui::Button("Ping all")) {
        app.PingAll();
    }
    HelpMarker("ICMP echo to each host. Firewalls may block ping even when the proxy port is reachable.");
    ImGui::SameLine();
    if (ImGui::Button("Remove selected")) {
        app.RemoveSelected();
    }
    ImGui::SameLine();
    if (ImGui::Button("Import...")) {
        app.ImportServers();
    }
    HelpMarker("Replaces the current list with servers loaded from a text file (same format used for export).");
    ImGui::SameLine();
    if (ImGui::Button("Export...")) {
        app.ExportServers();
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Add HTTP proxy");
    ImGui::SetNextItemWidth(220);
    ImGui::InputText("Name", app.draftName, sizeof(app.draftName));
    ImGui::SetNextItemWidth(220);
    ImGui::InputText("Host / domain", app.draftHost, sizeof(app.draftHost));
    ImGui::SetNextItemWidth(120);
    ImGui::InputInt("Port", &app.draftPort);
    ImGui::SetNextItemWidth(160);
    ImGui::InputText("Group", app.draftGroup, sizeof(app.draftGroup));
    HelpMarker("Optional label used to filter/organize the server list, e.g. \"Work\" or \"Home\".");
    ImGui::SetNextItemWidth(180);
    ImGui::InputText("Proxy username", app.draftUsername, sizeof(app.draftUsername));
    ImGui::SetNextItemWidth(180);
    ImGui::InputText("Proxy password", app.draftPassword, sizeof(app.draftPassword), ImGuiInputTextFlags_Password);
    HelpMarker(
        "Optional. Stored in plain text in the local config file and pre-seeds WinINet's "
        "default proxy credentials on Connect, so a Basic/NTLM auth popup can be skipped "
        "for apps sharing this process. Leave blank if your proxy needs no login.");
    if (ImGui::Button("Save server")) {
        app.AddServerFromDraft();
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Connection");

    const ImVec4 connectedColor = app.connected ? ImVec4(0.35f, 0.90f, 0.55f, 1.0f)
                                                : ImVec4(0.85f, 0.40f, 0.40f, 1.0f);
    ImGui::TextColored(connectedColor, "%s", app.connected ? "CONNECTED" : "DISCONNECTED");
    ImGui::SameLine();
    ImGui::TextUnformatted(app.statusLine.c_str());

    if (app.connected) {
        if (ImGui::Button("Disconnect", ImVec2(160, 36))) {
            app.Disconnect(true);
        }
    } else {
        if (ImGui::Button("Connect", ImVec2(160, 36))) {
            app.ConnectSelected();
        }
    }
    HelpMarker(
        "Applies a system HTTP proxy through WinINet (Internet Options). "
        "This affects browsers and WinINet apps. SOCKS5 is not a WinINet system proxy.");

    const auto& t = app.traffic.Stats();
    ImGui::Text("Approx. NIC throughput  down %.1f kbps   up %.1f kbps", t.kbpsIn, t.kbpsOut);
    ImGui::TextDisabled("Counters are system-wide, not proxy-session exclusive.");

    if (!app.lastError.empty()) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.35f, 0.12f, 0.12f, 0.6f));
        ImGui::BeginChild("error_banner", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
        ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.45f, 1.0f), "Error");
        ImGui::SameLine();
        const bool dismissed = ImGui::SmallButton("Dismiss");
        ImGui::TextWrapped("%s", app.lastError.c_str());
        if (dismissed) {
            app.lastError.clear();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGui::End();
}

}  // namespace npm
