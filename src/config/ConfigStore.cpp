#include "config/ConfigStore.h"

#include <windows.h>
#include <objbase.h>
#include <knownfolders.h>
#include <shlobj.h>

#include <fstream>
#include <sstream>

namespace npm {
namespace {

std::string MakeId() {
    GUID guid{};
    CoCreateGuid(&guid);
    wchar_t buf[64]{};
    StringFromGUID2(guid, buf, 64);
    char narrow[64]{};
    WideCharToMultiByte(CP_UTF8, 0, buf, -1, narrow, 64, nullptr, nullptr);
    return std::string(narrow);
}

std::string Trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    const auto end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return {};
    }
    return s.substr(start, end - start + 1);
}

// Parses one "id|name|host|port[|group[|username[|password]]]" line.
// Older config files only have the first four fields; the trailing ones
// default to empty so existing servers.txt files keep working.
bool ParseServerLine(const std::string& rawLine, ServerConfig& out) {
    const std::string line = Trim(rawLine);
    if (line.empty() || line[0] == '#') {
        return false;
    }
    std::stringstream ss(line);
    std::string id, name, host, portStr, group, username, password;
    if (!std::getline(ss, id, '|') || !std::getline(ss, name, '|') ||
        !std::getline(ss, host, '|') || !std::getline(ss, portStr, '|')) {
        return false;
    }
    std::getline(ss, group, '|');
    std::getline(ss, username, '|');
    std::getline(ss, password, '|');

    ServerConfig cfg;
    cfg.id = Trim(id);
    cfg.name = Trim(name);
    cfg.host = Trim(host);
    try {
        const int port = std::stoi(Trim(portStr));
        if (port <= 0 || port > 65535) {
            return false;
        }
        cfg.port = static_cast<uint16_t>(port);
    } catch (...) {
        return false;
    }
    cfg.group = Trim(group);
    cfg.username = Trim(username);
    cfg.password = password;  // don't trim: password could legitimately have edge spaces
    if (cfg.id.empty()) {
        cfg.id = MakeId();
    }
    out = std::move(cfg);
    return true;
}

void WriteServerLines(std::ostream& out, const std::vector<ServerConfig>& servers) {
    out << "# id|name|host|port|group|username|password\n";
    for (const auto& s : servers) {
        out << s.id << '|' << s.name << '|' << s.host << '|' << s.port << '|'
            << s.group << '|' << s.username << '|' << s.password << '\n';
    }
}

}  // namespace

std::filesystem::path ConfigStore::AppDataDir() {
    wchar_t* appdata = nullptr;
    std::filesystem::path dir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdata))) {
        dir = std::filesystem::path(appdata) / L"NetProxyManager";
        CoTaskMemFree(appdata);
    } else {
        dir = std::filesystem::temp_directory_path() / "NetProxyManager";
    }
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

std::filesystem::path ConfigStore::ServersPath() {
    return AppDataDir() / "servers.txt";
}

std::filesystem::path ConfigStore::WindowStatePath() {
    return AppDataDir() / "window.txt";
}

std::vector<ServerConfig> ConfigStore::Load() {
    std::vector<ServerConfig> servers;
    std::ifstream in(ServersPath());
    if (!in) {
        ServerConfig sample;
        sample.id = MakeId();
        sample.name = "Example HTTP proxy";
        sample.host = "127.0.0.1";
        sample.port = 7890;
        servers.push_back(std::move(sample));
        Save(servers);
        return servers;
    }

    std::string line;
    while (std::getline(in, line)) {
        ServerConfig cfg;
        if (ParseServerLine(line, cfg)) {
            servers.push_back(std::move(cfg));
        }
    }
    return servers;
}

bool ConfigStore::Save(const std::vector<ServerConfig>& servers) {
    std::ofstream out(ServersPath(), std::ios::trunc);
    if (!out) {
        return false;
    }
    WriteServerLines(out, servers);
    return true;
}

bool ConfigStore::ExportTo(const std::filesystem::path& path, const std::vector<ServerConfig>& servers) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        return false;
    }
    WriteServerLines(out, servers);
    return true;
}

bool ConfigStore::ImportFrom(const std::filesystem::path& path, std::vector<ServerConfig>& outServers, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "Could not open file for reading.";
        return false;
    }
    std::vector<ServerConfig> imported;
    std::string line;
    while (std::getline(in, line)) {
        ServerConfig cfg;
        if (ParseServerLine(line, cfg)) {
            imported.push_back(std::move(cfg));
        }
    }
    if (imported.empty()) {
        error = "No valid server entries found in file.";
        return false;
    }
    outServers = std::move(imported);
    return true;
}

WindowRect ConfigStore::LoadWindowRect() {
    WindowRect rect;
    std::ifstream in(WindowStatePath());
    if (!in) {
        return rect;
    }
    std::string line;
    if (std::getline(in, line)) {
        line = Trim(line);
        std::stringstream ss(line);
        std::string xs, ys, ws, hs;
        if (std::getline(ss, xs, '|') && std::getline(ss, ys, '|') &&
            std::getline(ss, ws, '|') && std::getline(ss, hs, '|')) {
            try {
                const int x = std::stoi(xs);
                const int y = std::stoi(ys);
                const int w = std::stoi(ws);
                const int h = std::stoi(hs);
                if (w >= 400 && h >= 300) {
                    rect.x = x;
                    rect.y = y;
                    rect.width = w;
                    rect.height = h;
                }
            } catch (...) {
                // keep defaults on malformed data
            }
        }
    }
    return rect;
}

bool ConfigStore::SaveWindowRect(const WindowRect& rect) {
    std::ofstream out(WindowStatePath(), std::ios::trunc);
    if (!out) {
        return false;
    }
    out << rect.x << '|' << rect.y << '|' << rect.width << '|' << rect.height << '\n';
    return true;
}

}  // namespace npm
