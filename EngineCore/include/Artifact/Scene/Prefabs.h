#pragma once

#include <string>

#include <glm/vec3.hpp>

#include "Artifact/Scene/SceneContext.h"

namespace Artifact::Scene::Prefabs {
    struct CameraPrefabOptions {
        std::string name{"Main Camera"};
        glm::vec3 position{0.0f, 5.0f, -10.0f};
        glm::vec3 rotation{20.0f, 0.0f, 0.0f};
        std::string scriptName{};
        bool autoReloadScript{true};
    };

    struct LuaActorPrefabOptions {
        std::string name{"LuaActor"};
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        std::string scriptName{};
        bool autoReloadScript{true};
    };

    entt::entity CreateMainCamera(SceneContext &scene, const CameraPrefabOptions &options = {});

    entt::entity CreateLuaActor(SceneContext &scene, const LuaActorPrefabOptions &options = {});
} // namespace Artifact::Scene::Prefabs
