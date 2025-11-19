#include "Artifact/Core/Paths.h"

#include <cstdlib>

#ifndef ARTIFACT_PROJECT_ROOT
#define ARTIFACT_PROJECT_ROOT ""
#endif

namespace Artifact::Core::Paths {

std::filesystem::path ProjectRoot() {
    const auto path = std::filesystem::path(ARTIFACT_PROJECT_ROOT);
    return std::filesystem::weakly_canonical(path);
}

std::filesystem::path AssetsRoot() {
    return ProjectRoot() / "Assets";
}

std::filesystem::path ResolveAssetPath(std::string_view relativePath) {
    std::filesystem::path candidate(relativePath);
    if (candidate.is_absolute()) {
        return std::filesystem::weakly_canonical(candidate);
    }
    return std::filesystem::weakly_canonical(AssetsRoot() / candidate);
}

} // namespace Artifact::Core::Paths

