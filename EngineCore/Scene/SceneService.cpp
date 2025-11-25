#include "Artifact/Scene/SceneService.h"

#include <algorithm>

#include "Artifact/Scene/Prefabs.h"
#include "Artifact/Scripting/ScriptAssetService.h"
#include "Artifact/Scene/ComponentRegistry.h"

namespace Artifact::Scene {
    namespace {
        void AttachChild(SceneContext &scene, entt::entity parent, entt::entity child);

        void DetachFromParent(SceneContext &scene, entt::entity entity);
    }

    SceneService::SceneService() {
        RegisterDefaultComponents(GetGlobalComponentRegistry());
        resetScene();
    }

    SceneService::~SceneService() = default;

    SceneContext &SceneService::context() {
        return m_scene;
    }

    const SceneContext &SceneService::context() const {
        return m_scene;
    }

    entt::entity SceneService::createEntity(const std::string &name) {
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

        const auto &nameComponent = m_scene.registry.get<NameComponent>(source);
        const entt::entity duplicate = createEntity(nameComponent.name + " Copy");

        if (auto *transform = m_scene.registry.try_get<TransformComponent>(source)) {
            auto &duplicateTransform = m_scene.registry.get<TransformComponent>(duplicate);
            duplicateTransform.transform = transform->transform;
        }

        if (auto *script = m_scene.registry.try_get<LuaScriptComponent>(source)) {
            auto &duplicateScript = m_scene.registry.get<LuaScriptComponent>(duplicate);
            duplicateScript.scriptName = script->scriptName;
            duplicateScript.enabled = script->enabled;
            duplicateScript.autoReloadScript = script->autoReloadScript;
            duplicateScript.serializedFields = script->serializedFields;
        }

        if (auto *active = m_scene.registry.try_get<ActiveComponent>(source)) {
            auto &duplicateActive = m_scene.registry.get<ActiveComponent>(duplicate);
            duplicateActive.active = active->active;
        }

        AttachChild(m_scene, source, duplicate);

        return duplicate;
    }

    void SceneService::destroyEntity(entt::entity entity) {
        if (!m_scene.registry.valid(entity)) {
            return;
        }

        auto &transform = m_scene.registry.get<TransformComponent>(entity);
        entt::entity child = transform.firstChild;
        while (child != entt::null) {
            entt::entity next = m_scene.registry.get<TransformComponent>(child).nextSibling;
            destroyEntity(child);
            child = next;
        }

        DetachFromParent(m_scene, entity);

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

    void SceneService::createDefaultScene(const Scripting::ScriptAssetService *scripts) {
        resetScene();
        entt::entity camera = createDefaultCamera();
        entt::entity actor = createDefaultLuaActor();

        if (scripts && !scripts->scripts().empty()) {
            if (auto *script = m_scene.registry.try_get<LuaScriptComponent>(camera)) {
                script->scriptName = scripts->scripts().front().name;
            }
            if (auto *script = m_scene.registry.try_get<LuaScriptComponent>(actor)) {
                script->scriptName = scripts->scripts().front().name;
            }
        }

        selectEntity(actor);
    }

    namespace {
        void AttachChild(SceneContext &scene, entt::entity parent, entt::entity child) {
            auto &childTransform = scene.registry.get<TransformComponent>(child);
            childTransform.parent = parent;
            childTransform.previousSibling = entt::null;
            childTransform.nextSibling = entt::null;

            if (parent == entt::null) {
                return;
            }

            auto &parentTransform = scene.registry.get<TransformComponent>(parent);
            if (parentTransform.firstChild != entt::null) {
                auto &firstChild = scene.registry.get<TransformComponent>(parentTransform.firstChild);
                childTransform.nextSibling = parentTransform.firstChild;
                firstChild.previousSibling = child;
            }
            parentTransform.firstChild = child;
        }

        void DetachFromParent(SceneContext &scene, entt::entity entity) {
            auto &transform = scene.registry.get<TransformComponent>(entity);
            if (transform.parent == entt::null) {
                return;
            }

            auto &parentTransform = scene.registry.get<TransformComponent>(transform.parent);
            if (parentTransform.firstChild == entity) {
                parentTransform.firstChild = transform.nextSibling;
            }
            if (transform.previousSibling != entt::null) {
                scene.registry.get<TransformComponent>(transform.previousSibling).nextSibling = transform.nextSibling;
            }
            if (transform.nextSibling != entt::null) {
                scene.registry.get<TransformComponent>(transform.nextSibling).previousSibling = transform.
                        previousSibling;
            }

            transform.parent = entt::null;
            transform.previousSibling = entt::null;
            transform.nextSibling = entt::null;
        }
    }

    void SceneService::setParent(entt::entity entity, entt::entity parent) {
        if (!m_scene.registry.valid(entity)) {
            return;
        }
        if (entity == parent) {
            return;
        }
        DetachFromParent(m_scene, entity);
        AttachChild(m_scene, parent, entity);
    }
} // namespace Artifact::Scene
