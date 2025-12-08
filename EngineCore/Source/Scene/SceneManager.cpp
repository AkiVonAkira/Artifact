#include "Artifact/Scene/SceneManager.h"
#include "Artifact/Scene/Prefabs.h"
#include "Artifact/Scene/ComponentRegistry.h"
#include "Artifact/Core/LogManager.h"
#include "Artifact/Core/Macros.h"

namespace Artifact::Scene {
    namespace {
        void AttachChild(SceneContext &scene, entt::entity parent, entt::entity child);

        void DetachFromParent(SceneContext &scene, entt::entity entity);
    }

    SceneManager::SceneManager() {
        RegisterDefaultComponents(GetGlobalComponentRegistry());
        ResetScene();
    }

    void SceneManager::Initialize() {
        if (m_initialized) {
            LOG_WARN("SceneManager already initialized");
            return;
        }

        LOG_INFO("Initializing SceneManager...");
        ResetScene();
        m_initialized = true;
        LOG_INFO("SceneManager initialized successfully");
    }

    void SceneManager::Shutdown() {
        if (!m_initialized) {
            return;
        }

        LOG_INFO("Shutting down SceneManager...");
        ResetScene();
        m_initialized = false;
        LOG_INFO("SceneManager shut down");
    }

    // ============================================================================
    // Entity Management
    // ============================================================================

    entt::entity SceneManager::CreateEntity(const std::string &name) {
        return Scene::CreateEntity(m_scene, name);
    }

    entt::entity SceneManager::CreateDefaultCamera() {
        return Prefabs::CreateMainCamera(m_scene, {});
    }

    entt::entity SceneManager::CreateDefaultLuaActor() {
        return Prefabs::CreateLuaActor(m_scene, {});
    }

    entt::entity SceneManager::DuplicateEntity(entt::entity source) {
        if (!m_scene.registry.valid(source)) {
            return entt::null;
        }

        const auto &nameComponent = m_scene.registry.get<NameComponent>(source);
        const entt::entity duplicate = CreateEntity(nameComponent.name + " Copy");

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

    void SceneManager::DestroyEntity(entt::entity entity) {
        if (!m_scene.registry.valid(entity)) {
            return;
        }

        auto &transform = m_scene.registry.get<TransformComponent>(entity);
        entt::entity child = transform.firstChild;
        while (child != entt::null) {
            entt::entity next = m_scene.registry.get<TransformComponent>(child).nextSibling;
            DestroyEntity(child);
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

    void SceneManager::SetParent(entt::entity entity, entt::entity parent) {
        if (!m_scene.registry.valid(entity)) {
            return;
        }

        DetachFromParent(m_scene, entity);

        if (parent != entt::null && m_scene.registry.valid(parent)) {
            AttachChild(m_scene, parent, entity);
        }
    }

    // ============================================================================
    // Selection Management
    // ============================================================================

    void SceneManager::SelectEntity(entt::entity entity) {
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

    entt::entity SceneManager::GetSelectedEntity() const {
        if (m_selected != entt::null && !m_scene.registry.valid(m_selected)) {
            return entt::null;
        }
        return m_selected;
    }

    void SceneManager::OnSelectionChanged(SelectionChangedCallback callback) {
        m_selectionChanged = std::move(callback);
    }

    // ============================================================================
    // Scene Management
    // ============================================================================

    void SceneManager::ResetScene() {
        Scene::ResetScene(m_scene);
        m_selected = entt::null;
    }

    void SceneManager::CreateDefaultScene() {
        ResetScene();
        entt::entity camera = CreateDefaultCamera();
        entt::entity actor = CreateDefaultLuaActor();

        // Lua scripting removed - no script assignment needed

        SelectEntity(actor);
    }

    // ============================================================================
    // Helper Functions (from SceneService)
    // ============================================================================

    namespace {
        void AttachChild(SceneContext &scene, entt::entity parent, entt::entity child) {
            if (parent == entt::null || child == entt::null) {
                return;
            }
            if (!scene.registry.valid(parent) || !scene.registry.valid(child)) {
                return;
            }

            auto &parentTransform = scene.registry.get<TransformComponent>(parent);
            auto &childTransform = scene.registry.get<TransformComponent>(child);

            childTransform.parent = parent;
            childTransform.nextSibling = parentTransform.firstChild;
            parentTransform.firstChild = child;
        }

        void DetachFromParent(SceneContext &scene, entt::entity entity) {
            if (entity == entt::null || !scene.registry.valid(entity)) {
                return;
            }

            auto &transform = scene.registry.get<TransformComponent>(entity);
            entt::entity parent = transform.parent;

            if (parent == entt::null || !scene.registry.valid(parent)) {
                transform.parent = entt::null;
                return;
            }

            auto &parentTransform = scene.registry.get<TransformComponent>(parent);

            if (parentTransform.firstChild == entity) {
                parentTransform.firstChild = transform.nextSibling;
            } else {
                entt::entity sibling = parentTransform.firstChild;
                while (sibling != entt::null) {
                    auto &siblingTransform = scene.registry.get<TransformComponent>(sibling);
                    if (siblingTransform.nextSibling == entity) {
                        siblingTransform.nextSibling = transform.nextSibling;
                        break;
                    }
                    sibling = siblingTransform.nextSibling;
                }
            }

            transform.parent = entt::null;
            transform.nextSibling = entt::null;
        }
    } // namespace anonymous
} // namespace Artifact::Scene
