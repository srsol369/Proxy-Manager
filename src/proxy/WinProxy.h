#pragma once

#include "common/Types.h"

#include <string>

namespace npm {

class WinProxy {
public:
    // Applies HTTP(S) proxy for WinINet-aware apps (Edge, Chrome, many Win32 clients).
    // username/password are optional; when set they are pre-seeded as the process's
    // default WinINet proxy credentials so a Basic/NTLM auth prompt can be skipped
    // for apps that share this process's WinINet cache. This does NOT make the
    // credentials available system-wide to other processes.
    static bool ApplyHttpProxy(const std::string& host, uint16_t port, std::string& error,
                                const std::string& username = {}, const std::string& password = {});
    static bool ClearProxy(std::string& error);
    static bool Query(ProxySnapshot& out, std::string& error);

    // Saves current OS proxy so Disconnect can restore it instead of forcing Direct.
    static bool CaptureCurrent(ProxySnapshot& out, std::string& error);
    static bool Restore(const ProxySnapshot& snapshot, std::string& error);

private:
    static bool SetPerConnection(const ProxySnapshot& snapshot, std::string& error);
    static bool NotifySettingsChanged();
    static void SetDefaultCredentials(const std::string& username, const std::string& password);
};

}  // namespace npm
