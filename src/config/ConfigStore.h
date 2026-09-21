#pragma once

#include "common/Types.h"

#include <filesystem>
#include <string>
#include <vector>

namespace npm {

class ConfigStore {
public:
    static std::filesystem::path AppDataDir();
    static std::filesystem::path ServersPath();
    static std::filesystem::path WindowStatePath();

    static std::vector<ServerConfig> Load();
    static bool Save(const std::vector<ServerConfig>& servers);

    // Import/Export use the same pipe-delimited format as the on-disk config,
    // so exported files can be shared and re-imported (or edited by hand).
    static bool ExportTo(const std::filesystem::path& path, const std::vector<ServerConfig>& servers);
    static bool ImportFrom(const std::filesystem::path& path, std::vector<ServerConfig>& outServers, std::string& error);

    static WindowRect LoadWindowRect();
    static bool SaveWindowRect(const WindowRect& rect);
};

}  // namespace npm
