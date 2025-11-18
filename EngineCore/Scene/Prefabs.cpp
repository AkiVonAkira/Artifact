#include "Artifact/Scene/Prefabs.h"

#include "Artifact/Scene/Components.h"

namespace Artifact::Scene::Prefabs {

entt::entity CreateMainCamera(SceneContext& scene, const CameraPrefabOptions& options) {
    auto entity = CreateEntity(scene, options.name);

    if (auto* transform = scene.registry.try_get<TransformComponent>(entity)) {
        transform->transform.position = options.position;
        transform->transform.rotation = options.rotation;
    }

    if (auto* script = scene.registry.try_get<LuaScriptComponent>(entity)) {
        script->scriptName = options.scriptName;
        script->autoReloadScript = options.autoReloadScript;
    }

    return entity;
}

entt::entity CreateLuaActor(SceneContext& scene, const LuaActorPrefabOptions& options) {
    auto entity = CreateEntity(scene, options.name);

    if (auto* transform = scene.registry.try_get<TransformComponent>(entity)) {
        transform->transform.position = options.position;
    }

    if (auto* script = scene.registry.try_get<LuaScriptComponent>(entity)) {
        script->scriptName = options.scriptName;
        script->autoReloadScript = options.autoReloadScript;
    }

    return entity;
}

} // namespace Artifact::Scene::Prefabs
