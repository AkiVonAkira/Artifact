#pragma once

#include <memory>
#include <spdlog/common.h>
#include <vector>
#include <string>
#include "EditorState.h"
#include "EditorSceneManager.h"
#include "EditorViewportManager.h"
#include "Artifact/Core/Config.h"
#include "Artifact/Scene/Components.h"
#include "Artifact/Scene/SceneContext.h"
#include "Panels/Core/PanelRegistry.h"
#include "Panels/Core/PanelInstances.h"

namespace Artifact::Core {
    class Application;
    class PlayModeController;
}


namespace Artifact::Scene {
    struct SceneContext;
}


namespace Artifact::Editor {
    class EditorCameraController;

    class EditorLayer {
    public:
        static const ImVec4 kInfoColor;
        static const ImVec4 kWarningColor;
        static const ImVec4 kErrorColor;

        explicit EditorLayer(Artifact::Core::Application &application);

        ~EditorLayer();

        void drawPlayControlsInMenuBar();

        void drawStatusBarContent();

        void createDefaultScene();

        std::string trimString(const std::string &value) const;

        std::string requestMeshAsset(const std::string &currentPath);

        std::string requestTextureAsset(const std::string &currentPath);

        void pushConsoleMessage(const std::string &message, spdlog::level::level_enum level = spdlog::level::info);

        void enterPlayMode();

        void stopPlayMode();

        void stepOnce();

        void requestExit();

        void resetLayout();

        // Legacy accessors
        Core::PlayModeController &playMode() const;

        const std::vector<PanelDescriptor> &panelDescriptors() const;

        void openPanel(const std::string &id, bool forceNewInstance = false);

        // Manager accessors
        EditorSceneManager* sceneManager() const { return m_sceneManager.get(); }
        EditorViewportManager* viewportManager() const { return m_viewportManager.get(); }

        // Entity management convenience wrappers (delegate to EditorSceneManager)
        entt::entity addEntity(const std::string& name);
        void duplicateEntity(entt::entity entity);
        void removeSelectedEntity();
        void selectEntity(entt::entity entity);
        entt::entity getSelectedEntity() const;

    private:
        void registerPanels();

        void rebuildPanelDescriptors();

        Core::Application &m_application;
        Core::RuntimeConfig m_runtimeConfig{};
        Core::PlayModeController *m_playMode{nullptr}; // TODO: Migrate to singleton

        EditorState m_state;

        // Managers
        std::unique_ptr<EditorSceneManager> m_sceneManager;
        std::unique_ptr<EditorViewportManager> m_viewportManager;

        Rendering::Camera *m_runtimeCamera{nullptr};
        Scene::SceneContext m_runtimeScene;
        bool m_runtimeActive{false};
        PanelManager m_panelManager;
        std::shared_ptr<spdlog::sinks::sink> m_consoleSink;
    };
}
