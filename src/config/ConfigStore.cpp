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

// ---------------------------------------------------------------------
// .sp file format
//
// Line 1 : "NPMSP1"                     <- magic/version header
// Line 2+: id|name|host|port|kind       <- one proxy per line
//   - fields are pipe-delimited, same escaping rule as the internal
//     servers.txt store (no '|' allowed inside a field)
//   - "kind" is a small integer matching ProxyKind (currently only 0 = Http)
//   - blank lines and lines starting with '#' are ignored
//
// Example:
//   NPMSP1
//   {8F62...}|Home|127.0.0.1|8080|0
//   {A013...}|Work VPN|10.0.0.5|3128|0
// ---------------------------------------------------------------------
constexpr const char* kSpMagic = "NPMSP1";

std::string EscapeField(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        if (ch == '|' || ch == '\n' || ch == '\r') {
            continue;  // field separators/newlines cannot appear in a field
        }
        out.push_back(ch);
    }
    return out;
}

int ProxyKindToInt(ProxyKind kind) {
    switch (kind) {
        case ProxyKind::Http:
        default:
            return 0;
    }
}

ProxyKind ProxyKindFromInt(int value) {
    switch (value) {
        case 0:
        default:
            return ProxyKind::Http;
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

bool ConfigStore::ExportToSp(const std::filesystem::path& path, const std::vector<ServerConfig>& servers) {
    std::ofstream out(path, std::ios::trunc | std::ios::binary);
    if (!out) {
        return false;
    }

    out << kSpMagic << '\n';
    out << "# Net Proxy Manager export - id|name|host|port|kind\n";
    for (const auto& s : servers) {
        out << EscapeField(s.id) << '|' << EscapeField(s.name) << '|' << EscapeField(s.host) << '|'
            << s.port << '|' << ProxyKindToInt(s.kind) << '\n';
    }
    return static_cast<bool>(out);
}

std::vector<ServerConfig> ConfigStore::ImportFromSp(const std::filesystem::path& path, bool& ok) {
    ok = false;
    std::vector<ServerConfig> servers;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return servers;
    }

    std::string header;
    if (!std::getline(in, header)) {
        return servers;
    }
    header = Trim(header);
    if (header != kSpMagic) {
        // Not a recognizable .sp file.
        return servers;
    }

    std::string line;
    while (std::getline(in, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string id, name, host, portStr, kindStr;
        if (!std::getline(ss, id, '|') || !std::getline(ss, name, '|') ||
            !std::getline(ss, host, '|') || !std::getline(ss, portStr, '|')) {
            continue;  // malformed line, skip it
        }
        std::getline(ss, kindStr, '|');  // optional; may be absent on older exports

        ServerConfig cfg;
        cfg.id = Trim(id);
        cfg.name = Trim(name);
        cfg.host = Trim(host);
        if (cfg.host.empty()) {
            continue;
        }
        try {
            const int port = std::stoi(Trim(portStr));
            if (port <= 0 || port > 65535) {
                continue;
            }
            cfg.port = static_cast<uint16_t>(port);
        } catch (...) {
            continue;
        }
        try {
            cfg.kind = kindStr.empty() ? ProxyKind::Http : ProxyKindFromInt(std::stoi(Trim(kindStr)));
        } catch (...) {
            cfg.kind = ProxyKind::Http;
        }
        if (cfg.id.empty()) {
            cfg.id = MakeId();
        }
        if (cfg.name.empty()) {
            cfg.name = cfg.host;
        }
        servers.push_back(std::move(cfg));
    }

    ok = true;  // header was valid; an empty list is still a "successful" import
    return servers;
}

}  // namespace npm
