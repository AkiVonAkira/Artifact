#pragma once

#include <entt/entity/entity.hpp>
#include <string>
#include <optional>

namespace Artifact::Scene {
    class SceneService;
    class SceneContext;
}

namespace Artifact::Editor {

    /**
     * @brief Manages editor scene operations and entity selection
     *
     * Extracted from EditorLayer to separate scene/entity management concerns.
     * Handles entity creation, deletion, duplication, and selection tracking.
     */
    class EditorSceneManager {
    public:
        explicit EditorSceneManager(Scene::SceneService* sceneService);
        ~EditorSceneManager() = default;

        /**
         * @brief Add a new entity to the scene
         * @param name Entity name
         * @return Created entity handle
         */
        entt::entity addEntity(const std::string& name);

        /**
         * @brief Duplicate an existing entity
         * @param entity Entity to duplicate
         * @return Duplicated entity handle
         */
        entt::entity duplicateEntity(entt::entity entity);

        /**
         * @brief Remove the currently selected entity
         */
        void removeSelectedEntity();

        /**
         * @brief Select an entity
         * @param entity Entity to select
         */
        void selectEntity(entt::entity entity);

        /**
         * @brief Deselect current entity
         */
        void deselectEntity();

        /**
         * @brief Get the currently selected entity
         * @return Selected entity, or entt::null if none
         */
        entt::entity getSelectedEntity() const { return m_selectedEntity; }

        /**
         * @brief Check if an entity is selected
         */
        bool hasSelection() const { return m_selectedEntity != entt::null; }

        /**
         * @brief Get the scene service
         */
        Scene::SceneService* sceneService() const { return m_sceneService; }

    private:
        Scene::SceneService* m_sceneService;
        entt::entity m_selectedEntity = entt::null;
    };

} // namespace Artifact::Editor

