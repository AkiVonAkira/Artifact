#include "Artifact/Scene/SceneContext.h"

#include <string>

namespace Artifact::Scene {
    void ResetScene(SceneContext &scene) {
        scene.registry.clear();
        scene.nextEntityId = 1;
    }

    entt::entity CreateEntity(SceneContext &scene, std::string_view name) {
        const entt::entity entity = scene.registry.create();
        scene.registry.emplace<IdComponent>(entity, IdComponent{scene.nextEntityId++});

        auto &nameComponent = scene.registry.emplace<NameComponent>(entity);
        if (!name.empty()) {
            nameComponent.name.assign(name.begin(), name.end());
        }

        scene.registry.emplace<ActiveComponent>(entity, ActiveComponent{true});

        auto &transform = scene.registry.emplace<TransformComponent>(entity);
        transform.transform.scale = {1.0f, 1.0f, 1.0f};
        transform.parent = entt::null;
        transform.firstChild = entt::null;
        transform.nextSibling = entt::null;
        transform.previousSibling = entt::null;

        scene.registry.emplace<LuaScriptComponent>(entity);
        return entity;
    }
} // namespace Artifact::Scene
