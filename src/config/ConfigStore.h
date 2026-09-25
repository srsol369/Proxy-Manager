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

    static std::vector<ServerConfig> Load();
    static bool Save(const std::vector<ServerConfig>& servers);

    // Custom ".sp" proxy list format (see ConfigStore.cpp for the exact
    // layout). Export writes the given list; Import parses a .sp file and
    // returns the servers found in it (empty on failure — check 'ok').
    static bool ExportToSp(const std::filesystem::path& path, const std::vector<ServerConfig>& servers);
    static std::vector<ServerConfig> ImportFromSp(const std::filesystem::path& path, bool& ok);
};

}  // namespace npm
