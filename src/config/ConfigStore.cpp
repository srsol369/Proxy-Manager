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
        line = Trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::stringstream ss(line);
        std::string id, name, host, portStr;
        if (!std::getline(ss, id, '|') || !std::getline(ss, name, '|') ||
            !std::getline(ss, host, '|') || !std::getline(ss, portStr, '|')) {
            continue;
        }
        ServerConfig cfg;
        cfg.id = Trim(id);
        cfg.name = Trim(name);
        cfg.host = Trim(host);
        try {
            const int port = std::stoi(Trim(portStr));
            if (port <= 0 || port > 65535) {
                continue;
            }
            cfg.port = static_cast<uint16_t>(port);
        } catch (...) {
            continue;
        }
        if (cfg.id.empty()) {
            cfg.id = MakeId();
        }
        servers.push_back(std::move(cfg));
    }
    return servers;
}

bool ConfigStore::Save(const std::vector<ServerConfig>& servers) {
    std::ofstream out(ServersPath(), std::ios::trunc);
    if (!out) {
        return false;
    }
    out << "# id|name|host|port\n";
    for (const auto& s : servers) {
        out << s.id << '|' << s.name << '|' << s.host << '|' << s.port << '\n';
    }
    return true;
}

}  // namespace npm
