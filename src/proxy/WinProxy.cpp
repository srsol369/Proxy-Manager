#include "proxy/WinProxy.h"

#include <windows.h>
#include <wininet.h>

#include <array>
#include <sstream>
#include <vector>

namespace npm {
namespace {

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring out(static_cast<size_t>(needed), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, out.data(), needed);
    if (!out.empty() && out.back() == L'\0') {
        out.pop_back();
    }
    return out;
}

std::string WideToUtf8(const wchar_t* wide) {
    if (!wide || !*wide) {
        return {};
    }
    const int needed = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, out.data(), needed, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0') {
        out.pop_back();
    }
    return out;
}

std::string LastErrorText(DWORD code) {
    LPWSTR buffer = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        code,
        0,
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr);
    std::string text = buffer ? WideToUtf8(buffer) : "Unknown Win32 error";
    if (buffer) {
        LocalFree(buffer);
    }
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
        text.pop_back();
    }
    std::ostringstream oss;
    oss << text << " (" << code << ")";
    return oss.str();
}

}  // namespace

bool WinProxy::NotifySettingsChanged() {
    InternetSetOptionW(nullptr, INTERNET_OPTION_SETTINGS_CHANGED, nullptr, 0);
    InternetSetOptionW(nullptr, INTERNET_OPTION_REFRESH, nullptr, 0);
    return true;
}

bool WinProxy::SetPerConnection(const ProxySnapshot& snapshot, std::string& error) {
    INTERNET_PER_CONN_OPTIONW options[3]{};
    options[0].dwOption = INTERNET_PER_CONN_FLAGS;
    if (snapshot.flags != 0) {
        options[0].Value.dwValue = snapshot.flags;
    } else {
        options[0].Value.dwValue = snapshot.enabled
            ? (PROXY_TYPE_PROXY | PROXY_TYPE_DIRECT)
            : PROXY_TYPE_DIRECT;
    }

    std::wstring server = snapshot.server;
    std::wstring bypass = snapshot.bypass.empty() ? L"<local>" : snapshot.bypass;

    options[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    options[1].Value.pszValue = snapshot.enabled ? server.data() : const_cast<LPWSTR>(L"");

    options[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
    options[2].Value.pszValue = snapshot.enabled ? bypass.data() : const_cast<LPWSTR>(L"");

    INTERNET_PER_CONN_OPTION_LISTW list{};
    list.dwSize = sizeof(list);
    list.pszConnection = nullptr;  // LAN / default connection
    list.dwOptionCount = 3;
    list.dwOptionError = 0;
    list.pOptions = options;

    DWORD listSize = sizeof(list);
    if (!InternetSetOptionW(nullptr, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, listSize)) {
        error = "InternetSetOption(PER_CONNECTION_OPTION) failed: " + LastErrorText(GetLastError());
        return false;
    }

    NotifySettingsChanged();
    return true;
}

bool WinProxy::Query(ProxySnapshot& out, std::string& error) {
    INTERNET_PER_CONN_OPTIONW options[3]{};
    options[0].dwOption = INTERNET_PER_CONN_FLAGS;
    options[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    options[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;

    INTERNET_PER_CONN_OPTION_LISTW list{};
    list.dwSize = sizeof(list);
    list.pszConnection = nullptr;
    list.dwOptionCount = 3;
    list.pOptions = options;

    DWORD size = sizeof(list);
    if (!InternetQueryOptionW(nullptr, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &size)) {
        error = "InternetQueryOption failed: " + LastErrorText(GetLastError());
        return false;
    }

    out.flags = options[0].Value.dwValue;
    out.enabled = (options[0].Value.dwValue & PROXY_TYPE_PROXY) != 0;
    out.server = options[1].Value.pszValue ? options[1].Value.pszValue : L"";
    out.bypass = options[2].Value.pszValue ? options[2].Value.pszValue : L"";

    if (options[1].Value.pszValue) {
        GlobalFree(options[1].Value.pszValue);
    }
    if (options[2].Value.pszValue) {
        GlobalFree(options[2].Value.pszValue);
    }
    return true;
}

bool WinProxy::CaptureCurrent(ProxySnapshot& out, std::string& error) {
    return Query(out, error);
}

bool WinProxy::Restore(const ProxySnapshot& snapshot, std::string& error) {
    return SetPerConnection(snapshot, error);
}

bool WinProxy::ApplyHttpProxy(const std::string& host, uint16_t port, std::string& error,
                               const std::string& username, const std::string& password) {
    if (host.empty() || port == 0) {
        error = "Host and port are required.";
        return false;
    }

    std::wostringstream proxy;
    // WinINet accepts "host:port" or protocol-specific lists.
    proxy << Utf8ToWide(host) << L':' << port;

    ProxySnapshot snap;
    snap.enabled = true;
    snap.flags = PROXY_TYPE_PROXY | PROXY_TYPE_DIRECT;
    snap.server = proxy.str();
    snap.bypass = L"<local>;localhost;127.0.0.1";
    if (!SetPerConnection(snap, error)) {
        return false;
    }

    if (!username.empty()) {
        SetDefaultCredentials(username, password);
    }
    return true;
}

void WinProxy::SetDefaultCredentials(const std::string& username, const std::string& password) {
    // These options seed the process-wide default proxy credentials that WinINet
    // falls back to when a proxy returns 407 Proxy Authentication Required.
    // Best-effort only: some auth schemes (e.g. NTLM/Negotiate) ignore them and
    // rely on the logged-in Windows account instead.
    std::wstring wUser = Utf8ToWide(username);
    std::wstring wPass = Utf8ToWide(password);
    InternetSetOptionW(nullptr, INTERNET_OPTION_PROXY_USERNAME,
                        const_cast<wchar_t*>(wUser.c_str()),
                        static_cast<DWORD>((wUser.size() + 1) * sizeof(wchar_t)));
    InternetSetOptionW(nullptr, INTERNET_OPTION_PROXY_PASSWORD,
                        const_cast<wchar_t*>(wPass.c_str()),
                        static_cast<DWORD>((wPass.size() + 1) * sizeof(wchar_t)));
}

bool WinProxy::ClearProxy(std::string& error) {
    ProxySnapshot snap;
    snap.enabled = false;
    snap.flags = PROXY_TYPE_DIRECT;
    snap.server.clear();
    snap.bypass.clear();
    return SetPerConnection(snap, error);
}

}  // namespace npm
