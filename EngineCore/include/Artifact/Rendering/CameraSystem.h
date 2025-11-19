#pragma once

#include <entt/entt.hpp>

#include "Artifact/Rendering/Camera.h"
#include "Artifact/Scene/Components.h"

namespace Artifact::Rendering {

class CameraSystem {
public:
    CameraSystem() = default;

    void update(entt::registry& /*registry*/) {}

    Camera* getPrimaryCamera(entt::registry& registry);
};

} // namespace Artifact::Rendering

