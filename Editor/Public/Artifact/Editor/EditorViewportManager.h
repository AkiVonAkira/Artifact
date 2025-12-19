#pragma once

#include "Viewport/EditorCameraController.h"
#include "EditorState.h"
#include <imgui.h>
#include <memory>
#include <entt/entity/entity.hpp>

namespace Artifact::Scene {
    struct SceneContext;
}

namespace Artifact::Rendering {
    class Camera;
}

namespace Artifact::Core {
    class PlayModeController;
}

namespace Artifact::Editor {

    /**
     * @brief Manages editor viewport rendering and camera
     *
     * Extracted from EditorLayer to separate viewport rendering concerns.
     * Handles scene rendering, camera management, and viewport overlays.
     */
    class EditorViewportManager {
    public:
        explicit EditorViewportManager(EditorState& state);
        ~EditorViewportManager() = default;

        /**
         * @brief Render the scene viewport
         * @param viewportSize Viewport dimensions
         * @param sceneContext Scene context for rendering
         * @param playMode Play mode controller
         * @param runtimeScene Runtime scene (when in play mode)
         * @param runtimeActive Whether runtime scene is active
         */
        void renderSceneViewport(
            const ImVec2& viewportSize,
            Scene::SceneContext* sceneContext,
            Core::PlayModeController* playMode,
            const Scene::SceneContext* runtimeScene,
            bool runtimeActive,
            entt::entity selectedEntity = entt::null
        );

        /**
         * @brief Draw editor camera overlay (position, rotation info)
         */
        void drawEditorCameraOverlay(const ImVec2& viewportSize, bool viewportFocused, bool viewportHovered);

        /**
         * @brief Draw camera preview overlay (picture-in-picture)
         */
        void drawCameraPreviewOverlay(const ImVec2& viewportSize, Scene::SceneContext* sceneContext, entt::entity selectedEntity);

        /**
         * @brief Draw scene camera frustums in viewport
         */
        void drawSceneCameraFrustums(const ImVec2& panelMin, const ImVec2& panelSize, Scene::SceneContext* sceneContext);

        /**
         * @brief Draw viewport input hints
         */
        void drawViewportInputHints(const ImVec2& panelMin, const ImVec2& panelSize, bool focused, bool hovered);

        /**
         * @brief Get the active viewport camera
         */
        Rendering::Camera* activeViewportCamera(
            Core::PlayModeController* playMode,
            Rendering::Camera* runtimeCamera
        );

        /**
         * @brief Get the editor camera controller
         */
        EditorCameraController* editorCamera() { return m_editorCamera.get(); }

    private:
        EditorState& m_state;
        std::unique_ptr<EditorCameraController> m_editorCamera;
    };

} // namespace Artifact::Editor

