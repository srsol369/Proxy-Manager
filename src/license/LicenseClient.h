#pragma once

#include "common/Types.h"

#include <string>

namespace npm {

class LicenseClient {
public:
    static LicenseState Load();
    static bool SaveKey(const std::string& key);
    static LicenseState ValidateOffline(const std::string& key);

    // Placeholder for a future HTTPS API (WinHTTP). Disabled until you set a real endpoint.
    static LicenseState ValidateRemote(const std::string& key, const std::wstring& endpointUrl);
};

}  // namespace npm
