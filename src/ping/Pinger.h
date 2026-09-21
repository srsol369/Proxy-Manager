#pragma once

#include "common/Types.h"

#include <string>

namespace npm {

class Pinger {
public:
    // ICMP echo (requires ICMP permitted by firewall). Returns milliseconds or nullopt on failure.
    static std::optional<int> IcmpPing(const std::string& host, int timeoutMs = 1500);
    static void PingAll(std::vector<ServerConfig>& servers, int timeoutMs = 1500);
};

}  // namespace npm
