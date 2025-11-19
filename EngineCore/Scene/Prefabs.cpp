#include "Artifact/Scene/Prefabs.h"

#include "Artifact/Scene/Components.h"

namespace Artifact::Scene::Prefabs {

entt::entity CreateMainCamera(SceneContext& scene, const CameraPrefabOptions& options) {
    auto entity = CreateEntity(scene, options.name);

    if (auto* transform = scene.registry.try_get<TransformComponent>(entity)) {
        transform->transform.position = options.position;
        transform->transform.rotation = options.rotation;
    }

    auto& cameraComp = scene.registry.emplace<CameraComponent>(entity);
    cameraComp.isPrimary = true;
    cameraComp.camera.setPerspective(
        45.0f,
        16.0f / 9.0f,
        0.1f,
        1000.0f
    );

    return entity;
}

entt::entity CreateLuaActor(SceneContext& scene, const LuaActorPrefabOptions& options) {
    auto entity = CreateEntity(scene, options.name);

    if (auto* transform = scene.registry.try_get<TransformComponent>(entity)) {
        transform->transform.position = options.position;
    }

    if (auto* script = scene.registry.try_get<LuaScriptComponent>(entity)) {
        script->scriptName = options.scriptName;
        script->enabled = true;
        script->autoReloadScript = options.autoReloadScript;
        script->serializedFields.clear();
    }

    return entity;
}

} // namespace Artifact::Scene::Prefabs
