#pragma once

#include <filesystem>
#include <string>

namespace Artifact::Core {

struct PathsConfig {
    std::filesystem::path rawAssets{std::filesystem::path{"Assets"} / "Raw"};
    std::filesystem::path cookedAssets{std::filesystem::path{"Assets"} / "Cooked"};
    std::filesystem::path shaderRoot{std::filesystem::path{"Assets"} / "Shaders"};
};

struct PackagingConfig {
    bool useLooseDirectories{true};
    bool signPackages{false};
    std::string signingKeyPath{};
};

struct RuntimeConfig {
    PathsConfig paths{};
    PackagingConfig packaging{};
};

RuntimeConfig LoadRuntimeConfig();

}
