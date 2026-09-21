#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace npm {

enum class ProxyKind {
    Http,
};

struct ServerConfig {
    std::string id;
    std::string name;
    std::string host;
    uint16_t port = 8080;
    ProxyKind kind = ProxyKind::Http;
    std::string group;       // free-form profile/group label, empty = "Ungrouped"
    std::string username;    // optional proxy auth username
    std::string password;    // optional proxy auth password (stored in plain text, see README)
    std::optional<int> lastDelayMs;
    bool lastPingOk = false;
};

struct WindowRect {
    int x = 120;
    int y = 120;
    int width = 920;
    int height = 720;
};

struct ProxySnapshot {
    bool enabled = false;
    std::wstring server;
    std::wstring bypass;
    uint32_t flags = 0;
};

}  // namespace npm
