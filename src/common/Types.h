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
    std::optional<int> lastDelayMs;
    bool lastPingOk = false;
};

struct ProxySnapshot {
    bool enabled = false;
    std::wstring server;
    std::wstring bypass;
    uint32_t flags = 0;
};

}  // namespace npm
