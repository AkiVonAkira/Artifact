#pragma once

#include <filesystem>
#include <string_view>

namespace Artifact::Core::Paths {
    std::filesystem::path ProjectRoot();

    std::filesystem::path AssetsRoot();

    std::filesystem::path CookedAssetsRoot();

    std::filesystem::path ResolveAssetPath(std::string_view relativePath);

    std::filesystem::path GetCookedMeshPath(const std::filesystem::path& originalPath);
} // namespace Artifact::Core::Paths
