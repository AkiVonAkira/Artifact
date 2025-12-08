#include "Artifact/Core/Config.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <system_error>

namespace Artifact::Core {
    namespace {
        std::filesystem::path CanonicalIfExists(const std::filesystem::path &candidate) {
            std::error_code ec;
            if (!std::filesystem::exists(candidate)) {
                return {};
            }
            const auto canonical = std::filesystem::weakly_canonical(candidate, ec);
            return ec ? candidate : canonical;
        }

        std::filesystem::path ResolvePath(const std::filesystem::path &input) {
            if (input.empty()) {
                return input;
            }
            if (input.is_absolute()) {
                return CanonicalIfExists(input);
            }

#ifdef ARTIFACT_PROJECT_ROOT
            {
                const std::filesystem::path projectRoot{ARTIFACT_PROJECT_ROOT};
                const auto sourceCandidate = CanonicalIfExists(projectRoot / input);
                if (!sourceCandidate.empty()) {
                    return sourceCandidate;
                }
                // Even if the assets haven't been materialized yet, prefer the project-root
                // location so editors and tools operate on the checked-in data.
                return projectRoot / input;
            }
#else
            const auto cwdCandidate = CanonicalIfExists(std::filesystem::current_path() / input);
            if (!cwdCandidate.empty()) {
                return cwdCandidate;
            }
            return std::filesystem::current_path() / input;
#endif
        }

        void EnsureDirectory(const std::filesystem::path &path) {
            if (path.empty()) {
                return;
            }
            std::error_code ec;
            std::filesystem::create_directories(path, ec);
            if (ec) {
                spdlog::warn("Failed to ensure directory '{}': {}", path.string(), ec.message());
            }
        }
    } // namespace

    RuntimeConfig LoadRuntimeConfig() {
        RuntimeConfig config{};
        config.paths.rawAssets = ResolvePath(config.paths.rawAssets);
        config.paths.cookedAssets = ResolvePath(config.paths.cookedAssets);
        config.paths.shaderRoot = ResolvePath(config.paths.shaderRoot);

        EnsureDirectory(config.paths.rawAssets);
        EnsureDirectory(config.paths.cookedAssets);
        EnsureDirectory(config.paths.shaderRoot);

        spdlog::info(
            "Runtime configuration loaded. LooseDirs={} | RawAssets='{}' | CookedAssets='{}' | Shaders='{}'",
            config.packaging.useLooseDirectories,
            config.paths.rawAssets.string(),
            config.paths.cookedAssets.string(),
            config.paths.shaderRoot.string());
        return config;
    }
}
