#pragma once

#include <filesystem>
#include <string_view>

namespace Artifact::Core::Paths {
    std::filesystem::path ProjectRoot();

    std::filesystem::path AssetsRoot();

    std::filesystem::path ResolveAssetPath(std::string_view relativePath);
} // namespace Artifact::Core::Paths
