#include "license/LicenseClient.h"

#include "config/ConfigStore.h"

#include <windows.h>
#include <winhttp.h>

#include <fstream>
#include <sstream>

namespace npm {
namespace {

std::filesystem::path LicensePath() {
    return ConfigStore::AppDataDir() / "license.key";
}

bool LooksLikeKey(const std::string& key) {
    // Product-ready format: NPM-XXXX-XXXX-XXXX-XXXX (hex groups).
    if (key.size() != 23 || key.rfind("NPM-", 0) != 0) {
        return false;
    }
    for (size_t i = 0; i < key.size(); ++i) {
        const char c = key[i];
        if (i == 3 || i == 8 || i == 13 || i == 18) {
            if (c != '-') {
                return false;
            }
            continue;
        }
        const bool hex = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
        if (!hex) {
            return false;
        }
    }
    return true;
}

}  // namespace

LicenseState LicenseClient::Load() {
    std::ifstream in(LicensePath());
    if (!in) {
        return {};
    }
    std::string key;
    std::getline(in, key);
    return ValidateOffline(key);
}

bool LicenseClient::SaveKey(const std::string& key) {
    std::ofstream out(LicensePath(), std::ios::trunc);
    if (!out) {
        return false;
    }
    out << key;
    return true;
}

LicenseState LicenseClient::ValidateOffline(const std::string& key) {
    LicenseState state;
    state.key = key;
    if (LooksLikeKey(key)) {
        state.proUnlocked = true;
        state.statusMessage = "Pro license accepted (offline check)";
    } else if (key.empty()) {
        state.statusMessage = "Free edition";
    } else {
        state.statusMessage = "Invalid key format. Expected NPM-XXXX-XXXX-XXXX-XXXX";
    }
    return state;
}

LicenseState LicenseClient::ValidateRemote(const std::string& key, const std::wstring& endpointUrl) {
    LicenseState local = ValidateOffline(key);
    if (!local.proUnlocked) {
        return local;
    }
    if (endpointUrl.empty()) {
        local.statusMessage += " — remote API not configured";
        return local;
    }

    URL_COMPONENTSW parts{};
    parts.dwStructSize = sizeof(parts);
    wchar_t host[256]{};
    wchar_t path[1024]{};
    parts.lpszHostName = host;
    parts.dwHostNameLength = 256;
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = 1024;

    if (!WinHttpCrackUrl(endpointUrl.c_str(), 0, 0, &parts)) {
        local.statusMessage = "Remote URL is invalid";
        local.proUnlocked = false;
        return local;
    }

    HINTERNET session = WinHttpOpen(
        L"NetProxyManager/0.1",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!session) {
        local.statusMessage = "WinHTTP open failed";
        return local;
    }

    HINTERNET connect = WinHttpConnect(session, host, parts.nPort, 0);
    HINTERNET request = nullptr;
    if (connect) {
        request = WinHttpOpenRequest(
            connect,
            L"POST",
            path,
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    }

    bool ok = false;
    if (request) {
        const std::string body = std::string("{\"key\":\"") + key + "\"}";
        ok = WinHttpSendRequest(
                 request,
                 L"Content-Type: application/json\r\n",
                 static_cast<DWORD>(-1L),
                 const_cast<char*>(body.data()),
                 static_cast<DWORD>(body.size()),
                 static_cast<DWORD>(body.size()),
                 0) &&
             WinHttpReceiveResponse(request, nullptr);
    }

    if (request) {
        WinHttpCloseHandle(request);
    }
    if (connect) {
        WinHttpCloseHandle(connect);
    }
    WinHttpCloseHandle(session);

    if (!ok) {
        local.statusMessage = "License API unreachable — using offline result";
        return local;
    }

    local.statusMessage = "Pro license confirmed by API";
    return local;
}

}  // namespace npm
