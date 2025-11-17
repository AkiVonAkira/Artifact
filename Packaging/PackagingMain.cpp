#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <string_view>

namespace {
struct Manifest {
    std::filesystem::path cookedRoot{"Assets/Cooked"};
    std::filesystem::path output{"dist"};
    std::string packageName{"SampleGame"};
    std::string version{"0.1.0"};
};

Manifest ParseArgs(int argc, char** argv) {
    Manifest manifest;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--cooked" && i + 1 < argc) {
            manifest.cookedRoot = argv[++i];
        } else if (arg == "--out" && i + 1 < argc) {
            manifest.output = argv[++i];
        } else if (arg == "--name" && i + 1 < argc) {
            manifest.packageName = argv[++i];
        } else if (arg == "--version" && i + 1 < argc) {
            manifest.version = argv[++i];
        }
    }
    return manifest;
}

void WriteManifest(const Manifest& manifest, const std::filesystem::path& manifestPath) {
    std::ofstream out(manifestPath);
    out << "name=" << manifest.packageName << '\n';
    out << "version=" << manifest.version << '\n';
    out << "cooked=" << manifest.cookedRoot.string() << '\n';
}

}

int main(int argc, char** argv) {
    auto manifest = ParseArgs(argc, argv);
    std::filesystem::create_directories(manifest.output);
    const auto manifestPath = manifest.output / (manifest.packageName + ".artifactpkg.manifest");
    WriteManifest(manifest, manifestPath);
    spdlog::info("Wrote manifest {}", manifestPath.string());
    return 0;
}
