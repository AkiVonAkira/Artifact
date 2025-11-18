#include <catch2/catch_test_macros.hpp>

#include "Artifact/Core/Config.h"

TEST_CASE("runtime config loads defaults", "[config]") {
    auto cfg = Artifact::Core::LoadRuntimeConfig();
    REQUIRE(cfg.packaging.useLooseDirectories);
    REQUIRE(cfg.paths.rawAssets.filename() == "Raw");
}
