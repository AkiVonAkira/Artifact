#include "Artifact/Editor/EditorSceneManager.h"
#include "Artifact/Scene/Components.h"
#include "Artifact/Scene/SceneContext.h"
#include "Artifact/Core/LogManager.h"

namespace Artifact::Editor {

    EditorSceneManager::EditorSceneManager()
        : m_selectedEntity(entt::null) {
    }

    entt::entity EditorSceneManager::addEntity(const std::string& name) {
        auto& registry = m_sceneContext.registry;
        auto entity = registry.create();
        registry.emplace<Scene::IdComponent>(entity, static_cast<int>(entity));
        registry.emplace<Scene::NameComponent>(entity, name);
        registry.emplace<Scene::ActiveComponent>(entity, true);
        registry.emplace<Scene::TransformComponent>(entity);
        selectEntity(entity);
        return entity;
    }

    entt::entity EditorSceneManager::duplicateEntity(entt::entity entity) {
        if (entity == entt::null) {
            return entt::null;
        }
        auto& registry = m_sceneContext.registry;
        auto duplicate = registry.create();
        // Copy components from source to duplicate
        // TODO: Implement full component copy
        if (auto* name = registry.try_get<Scene::NameComponent>(entity)) {
            registry.emplace<Scene::NameComponent>(duplicate, name->name + " (Copy)");
        }
        if (auto* transform = registry.try_get<Scene::TransformComponent>(entity)) {
            registry.emplace<Scene::TransformComponent>(duplicate, *transform);
        }
        if (auto* active = registry.try_get<Scene::ActiveComponent>(entity)) {
            registry.emplace<Scene::ActiveComponent>(duplicate, *active);
        }
        selectEntity(duplicate);
        return duplicate;
    }

    void EditorSceneManager::removeSelectedEntity() {
        if (m_selectedEntity == entt::null) {
            return;
        }
        m_sceneContext.registry.destroy(m_selectedEntity);
        m_selectedEntity = entt::null;
    }

    void EditorSceneManager::selectEntity(entt::entity entity) {
        m_selectedEntity = entity;
    }

    void EditorSceneManager::deselectEntity() {
        m_selectedEntity = entt::null;
    }

} // namespace Artifact::Editor

