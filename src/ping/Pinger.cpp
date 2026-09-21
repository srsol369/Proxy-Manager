#include "ping/Pinger.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#include <algorithm>
#include <chrono>
#include <mutex>
#include <vector>

namespace npm {
namespace {

std::once_flag g_wsaOnce;

void EnsureWsa() {
    std::call_once(g_wsaOnce, [] {
        WSADATA data{};
        WSAStartup(MAKEWORD(2, 2), &data);
    });
}

bool ResolveIpv4(const std::string& host, IN_ADDR& addr) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &result) != 0 || !result) {
        return false;
    }
    auto* ipv4 = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    addr = ipv4->sin_addr;
    freeaddrinfo(result);
    return true;
}

}  // namespace

std::optional<int> Pinger::IcmpPing(const std::string& host, int timeoutMs) {
    EnsureWsa();

    IN_ADDR dest{};
    if (!ResolveIpv4(host, dest)) {
        return std::nullopt;
    }

    HANDLE icmp = IcmpCreateFile();
    if (icmp == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    constexpr int kPayloadSize = 32;
    unsigned char send[kPayloadSize]{};
    const DWORD replySize = sizeof(ICMP_ECHO_REPLY) + kPayloadSize + 8;
    std::vector<unsigned char> reply(replySize);

    const auto start = std::chrono::steady_clock::now();
    const DWORD count = IcmpSendEcho(
        icmp,
        dest.S_un.S_addr,
        send,
        kPayloadSize,
        nullptr,
        reply.data(),
        replySize,
        static_cast<DWORD>(timeoutMs));
    IcmpCloseHandle(icmp);

    if (count == 0) {
        return std::nullopt;
    }

    const auto* echo = reinterpret_cast<ICMP_ECHO_REPLY*>(reply.data());
    if (echo->Status != IP_SUCCESS) {
        return std::nullopt;
    }

    if (echo->RoundTripTime > 0) {
        return static_cast<int>(echo->RoundTripTime);
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    return static_cast<int>(elapsed.count());
}

void Pinger::PingAll(std::vector<ServerConfig>& servers, int timeoutMs) {
    for (auto& server : servers) {
        server.lastDelayMs = IcmpPing(server.host, timeoutMs);
        server.lastPingOk = server.lastDelayMs.has_value();
    }
    std::stable_sort(servers.begin(), servers.end(), [](const ServerConfig& a, const ServerConfig& b) {
        const int da = a.lastDelayMs.value_or(1'000'000);
        const int db = b.lastDelayMs.value_or(1'000'000);
        return da < db;
    });
}

}  // namespace npm
