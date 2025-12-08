// Created by Artifact Engine Team on 2025-11-25.
// Scene manager with singleton pattern

#pragma once

#include "Artifact/Core/Singleton.h"
#include "Artifact/Scene/SceneContext.h"

#include <functional>
#include <memory>
#include <string>
#include <entt/entt.hpp>


namespace Artifact::Scene {
    /**
     * @brief Centralized scene management system
     *
     * Manages the active scene, entity creation/destruction, and selection state.
     * Provides global access to the current scene context.
     */
    class SceneManager : public Core::Singleton<SceneManager> {
        friend class Core::Singleton<SceneManager>;

    public:
        using SelectionChangedCallback = std::function<void(entt::entity)>;

        /**
         * @brief Initialize the scene manager
         */
        void Initialize();

        /**
         * @brief Shutdown the scene manager
         */
        void Shutdown();

        /**
         * @brief Get the current scene context
         */
        SceneContext &GetContext() { return m_scene; }
        const SceneContext &GetContext() const { return m_scene; }

        // ========================================================================
        // Entity Management
        // ========================================================================

        /**
         * @brief Create a new entity
         * @param name Entity name
         * @return Created entity handle
         */
        entt::entity CreateEntity(const std::string &name);

        /**
         * @brief Create a default camera entity
         */
        entt::entity CreateDefaultCamera();

        /**
         * @brief Create a default Lua actor entity
         */
        entt::entity CreateDefaultLuaActor();

        /**
         * @brief Duplicate an entity
         * @param source Entity to duplicate
         * @return Duplicated entity handle
         */
        entt::entity DuplicateEntity(entt::entity source);

        /**
         * @brief Destroy an entity and all its children
         * @param entity Entity to destroy
         */
        void DestroyEntity(entt::entity entity);

        /**
         * @brief Set parent-child relationship
         * @param entity Child entity
         * @param parent Parent entity
         */
        void SetParent(entt::entity entity, entt::entity parent);

        // ========================================================================
        // Selection Management
        // ========================================================================

        /**
         * @brief Select an entity
         * @param entity Entity to select
         */
        void SelectEntity(entt::entity entity);

        /**
         * @brief Get currently selected entity
         */
        entt::entity GetSelectedEntity() const;

        /**
         * @brief Register callback for selection changes
         */
        void OnSelectionChanged(SelectionChangedCallback callback);

        // ========================================================================
        // Scene Management
        // ========================================================================

        /**
         * @brief Reset scene to empty state
         */
        void ResetScene();

        /**
         * @brief Create a default scene with camera and actor
         */
        void CreateDefaultScene();

        // ========================================================================

    private:
        SceneManager();

        ~SceneManager() = default;

        SceneContext m_scene;
        entt::entity m_selected{entt::null};
        SelectionChangedCallback m_selectionChanged;
        bool m_initialized = false;
    };
} // namespace Artifact::Scene
