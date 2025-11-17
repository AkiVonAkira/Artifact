#include "Artifact/Core/Config.h"

#include <spdlog/spdlog.h>

namespace Artifact::Core {

RuntimeConfig LoadRuntimeConfig() {
    RuntimeConfig config{};
    // TODO: Load from artifact.toml once available.
    spdlog::info("Runtime configuration loaded. LooseDirs={}", config.packaging.useLooseDirectories);
    return config;
}

}
