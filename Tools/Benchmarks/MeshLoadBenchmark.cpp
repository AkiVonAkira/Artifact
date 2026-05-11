#include "Artifact/Assets/MeshManager.h"
#include "Artifact/Core/Paths.h"
#include "Artifact/Core/LogManager.h"

#include <chrono>
#include <iostream>
#include <string>
#include <functional>

using namespace Artifact;
using namespace Artifact::Assets;
using namespace Artifact::Core;

static double MeasureMs(const std::function<void()>& fn, int iterations) {
    using clock = std::chrono::high_resolution_clock;
    fn(); // warm-up
    auto start = clock::now();
    for (int i = 0; i < iterations; ++i) {
        fn();
    }
    auto end = clock::now();
    std::chrono::duration<double, std::milli> total = end - start;
    return total.count() / iterations;
}

int main(int argc, char** argv) {
    // Choose a mesh path. Default to Assets/Models/Viking_House.obj or allow arg override.
    std::string relPath = "Assets/Models/Viking_House.obj";
    if (argc > 1) {
        relPath = argv[1];
    }

    const auto originalPath = Paths::ResolveAssetPath(relPath);
    const auto cookedPath = Paths::GetCookedMeshPath(originalPath);

    std::cout << "Artifact Mesh Load Benchmark\n";
    std::cout << "Raw:    " << originalPath.string() << "\n";
    std::cout << "Cooked: " << cookedPath.string() << "\n";

    MeshManager mgr;

    // Measure: raw load by forcing re-parse (delete cooked if exists temporarily)
    const int iterations = 20;

    // Lambda to load raw by bypassing cache: remove cooked file, then call loadMesh
    auto loadRawOnce = [&]() {
        if (std::filesystem::exists(cookedPath)) {
            std::filesystem::remove(cookedPath);
        }
        const Mesh* m = mgr.loadMesh(relPath);
        (void)m;
    };

    // Lambda to load cooked: ensure cooked exists by one parse, then load again (should hit binary)
    auto loadCookedOnce = [&]() {
        // First call ensures cooked is created if missing
        const Mesh* m = mgr.loadMesh(relPath);
        (void)m;
        // Next call should hit cache
        const Mesh* m2 = mgr.loadMesh(relPath);
        (void)m2;
    };

    // Raw: measure by clearing and reloading each time
    double rawMs = MeasureMs(loadRawOnce, iterations);

    // Cooked: measure by hitting binary cache each time (second load should be cached)
    double cookedMs = MeasureMs(loadCookedOnce, iterations);

    std::cout << "Iterations: " << iterations << "\n";
    std::cout << "Average Raw Load:    " << rawMs << " ms\n";
    std::cout << "Average Cooked Load: " << cookedMs << " ms\n";
    if (cookedMs > 0.0) {
        std::cout << "Speedup: " << (rawMs / cookedMs) << "x\n";
    }

    // Restore cooked file for engine use
    if (!std::filesystem::exists(cookedPath)) {
        // Trigger creation once
        (void)mgr.loadMesh(relPath);
    }

    return 0;
}
