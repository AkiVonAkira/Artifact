#include "Artifact/Editor/EditorSceneManager.h"
#include "Artifact/Scene/SceneService.h"
#include "Artifact/Scene/SceneContext.h"
#include "Artifact/Core/LogManager.h"

namespace Artifact::Editor {

    EditorSceneManager::EditorSceneManager(Scene::SceneService* sceneService)
        : m_sceneService(sceneService)
        , m_selectedEntity(entt::null) {
    }

    entt::entity EditorSceneManager::addEntity(const std::string& name) {
        if (!m_sceneService) {
            return entt::null;
        }
        auto entity = m_sceneService->createEntity(name);
        selectEntity(entity);
        return entity;
    }

    entt::entity EditorSceneManager::duplicateEntity(entt::entity entity) {
        if (!m_sceneService || entity == entt::null) {
            return entt::null;
        }
        auto duplicate = m_sceneService->duplicateEntity(entity);
        selectEntity(duplicate);
        return duplicate;
    }

    void EditorSceneManager::removeSelectedEntity() {
        if (!m_sceneService || m_selectedEntity == entt::null) {
            return;
        }
        m_sceneService->destroyEntity(m_selectedEntity);
        m_selectedEntity = entt::null;
    }

    void EditorSceneManager::selectEntity(entt::entity entity) {
        m_selectedEntity = entity;
        if (m_sceneService) {
            m_sceneService->selectEntity(entity);
        }
    }

    void EditorSceneManager::deselectEntity() {
        m_selectedEntity = entt::null;
        if (m_sceneService) {
            m_sceneService->selectEntity(entt::null);
        }
    }

} // namespace Artifact::Editor

