#pragma once

#include <string_view>

#include <entt/entt.hpp>

#include "Artifact/Scene/Components.h"

namespace Artifact::Scene {

struct SceneContext {
    entt::registry registry;
    int nextEntityId{1};
};

void ResetScene(SceneContext& scene);
entt::entity CreateEntity(SceneContext& scene, std::string_view name);

} // namespace Artifact::Scene
