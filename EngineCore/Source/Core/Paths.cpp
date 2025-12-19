#include "Artifact/Core/Paths.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace {
    std::filesystem::path StripAssetsPrefix(const std::filesystem::path &relativePath) {
        if (relativePath.empty()) {
            return relativePath;
        }

        auto normalized = relativePath.lexically_normal();
        auto it = normalized.begin();
        if (it == normalized.end()) {
            return normalized;
        }

        std::string firstComponent = it->string();
        std::string lowered = firstComponent;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (lowered == "assets") {
            std::filesystem::path remainder;
            for (++it; it != normalized.end(); ++it) {
                remainder /= *it;
            }
            return remainder;
        }

        return normalized;
    }
}

namespace Artifact::Core::Paths {
    std::filesystem::path ProjectRoot() {
        const auto path = std::filesystem::path(ARTIFACT_PROJECT_ROOT);
        return std::filesystem::weakly_canonical(path);
    }

    std::filesystem::path AssetsRoot() {
        return ProjectRoot() / "Assets";
    }

    std::filesystem::path CookedAssetsRoot() {
        return ProjectRoot() / "Assets" / "Cooked";
    }

    std::filesystem::path ResolveAssetPath(std::string_view relativePath) {
        if (relativePath.empty()) {
            return std::filesystem::weakly_canonical(AssetsRoot());
        }

        std::filesystem::path candidate(relativePath);
        if (candidate.is_absolute()) {
            return std::filesystem::weakly_canonical(candidate);
        }

        const auto sanitized = StripAssetsPrefix(candidate);
        std::filesystem::path absolute = AssetsRoot();
        if (!sanitized.empty()) {
            absolute /= sanitized;
        }
        return std::filesystem::weakly_canonical(absolute);
    }

    std::filesystem::path GetCookedMeshPath(const std::filesystem::path& originalPath) {
        // Convert original mesh path to cooked path
        // e.g., Assets/Models/cube.obj -> Assets/Cooked/Models/cube.mesh
        auto relativePath = std::filesystem::relative(originalPath, AssetsRoot());
        auto cookedPath = CookedAssetsRoot() / relativePath;
        cookedPath.replace_extension(".mesh");
        return cookedPath;
    }
} // namespace Artifact::Core::Paths
