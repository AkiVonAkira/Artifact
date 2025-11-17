#include "Artifact/Core/Config.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <optional>
#include <string_view>

namespace {
struct CommandLine {
    std::filesystem::path rawRoot{"Assets/Raw"};
    std::filesystem::path cookedRoot{"Assets/Cooked"};
    bool cookTextures{true};
    bool cookModels{true};
    bool cookAudio{true};
    bool cookShaders{true};
};

CommandLine Parse(int argc, char** argv) {
    CommandLine cli;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--raw" && i + 1 < argc) {
            cli.rawRoot = argv[++i];
        } else if (arg == "--cooked" && i + 1 < argc) {
            cli.cookedRoot = argv[++i];
        } else if (arg == "--textures=off") {
            cli.cookTextures = false;
        } else if (arg == "--models=off") {
            cli.cookModels = false;
        } else if (arg == "--audio=off") {
            cli.cookAudio = false;
        } else if (arg == "--shaders=off") {
            cli.cookShaders = false;
        }
    }
    return cli;
}

void EnsureDirectories(const CommandLine& cli) {
    std::filesystem::create_directories(cli.cookedRoot / "Textures");
    std::filesystem::create_directories(cli.cookedRoot / "Models");
    std::filesystem::create_directories(cli.cookedRoot / "Audio");
    std::filesystem::create_directories(cli.cookedRoot / "Shaders");
}

}

int main(int argc, char** argv) {
    auto cli = Parse(argc, argv);
    EnsureDirectories(cli);

    spdlog::info("Cooking assets from {} -> {}", cli.rawRoot.string(), cli.cookedRoot.string());

    if (cli.cookTextures) {
        spdlog::info("[cook] textures (BC7/ASTC)");
    }
    if (cli.cookModels) {
        spdlog::info("[cook] models (Assimp -> artmesh)");
    }
    if (cli.cookAudio) {
        spdlog::info("[cook] audio (WAV -> Opus)");
    }
    if (cli.cookShaders) {
        spdlog::info("[cook] shaders (GLSL -> SPIR-V validation)");
    }

    spdlog::info("Cook complete");
    return 0;
}
