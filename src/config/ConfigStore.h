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
};

}  // namespace npm
