#include "Artifact/Scene/SceneService.h"

#include <algorithm>

#include "Artifact/Scene/Prefabs.h"
#include "Artifact/Scripting/ScriptAssetService.h"

namespace Artifact::Scene {

SceneService::SceneService() {
    resetScene();
}

SceneService::~SceneService() = default;

SceneContext& SceneService::context() {
    return m_scene;
}

const SceneContext& SceneService::context() const {
    return m_scene;
}

entt::entity SceneService::createEntity(const std::string& name) {
    return CreateEntity(m_scene, name);
}

entt::entity SceneService::createDefaultCamera() {
    return Prefabs::CreateMainCamera(m_scene, {});
}

entt::entity SceneService::createDefaultLuaActor() {
    return Prefabs::CreateLuaActor(m_scene, {});
}

entt::entity SceneService::duplicateEntity(entt::entity source) {
    if (!m_scene.registry.valid(source)) {
        return entt::null;
    }

    const auto& nameComponent = m_scene.registry.get<NameComponent>(source);
    const entt::entity duplicate = createEntity(nameComponent.name + " Copy");

    if (auto* transform = m_scene.registry.try_get<TransformComponent>(source)) {
        auto& duplicateTransform = m_scene.registry.get<TransformComponent>(duplicate);
        duplicateTransform.transform = transform->transform;
    }

    if (auto* script = m_scene.registry.try_get<LuaScriptComponent>(source)) {
        auto& duplicateScript = m_scene.registry.get<LuaScriptComponent>(duplicate);
        duplicateScript.scriptName = script->scriptName;
        duplicateScript.enabled = script->enabled;
        duplicateScript.autoReloadScript = script->autoReloadScript;
        duplicateScript.serializedFields = script->serializedFields;
    }

    if (auto* active = m_scene.registry.try_get<ActiveComponent>(source)) {
        auto& duplicateActive = m_scene.registry.get<ActiveComponent>(duplicate);
        duplicateActive.active = active->active;
    }

    return duplicate;
}

void SceneService::destroyEntity(entt::entity entity) {
    if (!m_scene.registry.valid(entity)) {
        return;
    }

    m_scene.registry.destroy(entity);

    if (entity == m_selected) {
        m_selected = entt::null;
        auto view = m_scene.registry.view<IdComponent>();
        if (!view.empty()) {
            m_selected = *view.begin();
        }
        if (m_selectionChanged) {
            m_selectionChanged(m_selected);
        }
    }
}

void SceneService::selectEntity(entt::entity entity) {
    if (entity != entt::null && !m_scene.registry.valid(entity)) {
        entity = entt::null;
    }

    if (m_selected == entity) {
        return;
    }

    m_selected = entity;
    if (m_selectionChanged) {
        m_selectionChanged(m_selected);
    }
}

entt::entity SceneService::selectedEntity() const {
    if (m_selected != entt::null && !m_scene.registry.valid(m_selected)) {
        return entt::null;
    }
    return m_selected;
}

void SceneService::onSelectionChanged(SelectionChangedCallback callback) {
    m_selectionChanged = std::move(callback);
}

void SceneService::resetScene() {
    ResetScene(m_scene);
    m_selected = entt::null;
}

void SceneService::createDefaultScene(const Scripting::ScriptAssetService* scripts) {
    resetScene();
    entt::entity camera = createDefaultCamera();
    entt::entity actor = createDefaultLuaActor();

    if (scripts && !scripts->scripts().empty()) {
        if (auto* script = m_scene.registry.try_get<LuaScriptComponent>(camera)) {
            script->scriptName = scripts->scripts().front().name;
        }
        if (auto* script = m_scene.registry.try_get<LuaScriptComponent>(actor)) {
            script->scriptName = scripts->scripts().front().name;
        }
    }

    selectEntity(actor);
}

} // namespace Artifact::Scene
