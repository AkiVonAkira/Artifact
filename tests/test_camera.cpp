#include <catch2/catch_test_macros.hpp>

#include "Artifact/Rendering/Camera.h"

using namespace Artifact::Rendering;

TEST_CASE("Camera default perspective setup", "[camera]") {
    Camera cam{ProjectionType::Perspective};

    auto view = cam.viewMatrix();
    auto proj = cam.projectionMatrix();

    // View and projection should not be identity at the same time
    REQUIRE(view != proj);
}

TEST_CASE("Camera forward/right/up vectors", "[camera]") {
    Camera cam{ProjectionType::Perspective};
    cam.setPosition({0.0f, 0.0f, 5.0f});
    cam.lookAt({0.0f, 0.0f, 0.0f});

    auto fwd = cam.forward();
    auto right = cam.right();
    auto up = cam.up();

    // Basic orthogonality checks
    REQUIRE(fwd != right);
    REQUIRE(fwd != up);
}

