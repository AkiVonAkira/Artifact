#pragma once

#include <entt/entity/entity.hpp>

#include "EditorState.h"
#include "Artifact/Assets/AssetWatcher.h"
#include "Panels/PanelFactories.h"
#include "Artifact/Core/Config.h"
#include "Artifact/Scene/Components.h"
#include "Viewport/EditorCameraController.h"

namespace Artifact::Core {
class Application;
class PlayModeController;
}

namespace Artifact::Scripting {
class LuaContext;
class LuaRuntimeSystem;
class ScriptAssetService;
class LuaBindings;
struct ScriptAsset;
}

namespace Artifact::Scene {
class SceneService;
}

namespace Artifact::Rendering {
class Renderer;
}

namespace Artifact::Editor {
class EditorCameraController;

class EditorLayer {
public:
    static const ImVec4 kInfoColor;
    static const ImVec4 kWarningColor;
    static const ImVec4 kErrorColor;

    explicit EditorLayer(Artifact::Core::Application& application);

    void drawPlayControlsInMenuBar();
    void drawStatusBarContent();
    void renderSceneViewport(const ImVec2& viewportSize);

    void refreshScriptAssets(bool announce = true);
    void createDefaultScene();

    std::string trimString(const std::string& value) const;

    entt::entity addEntity(const std::string& name);
    void duplicateEntity(entt::entity entity);
    void removeSelectedEntity();
    void selectEntity(entt::entity entity);
    entt::entity getSelectedEntity() const;
    void assignScriptToSelection(const std::string& scriptName);

    bool createLuaScript(const std::string& scriptName);
    void pushConsoleMessage(const std::string& message, const ImVec4& color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    void executeLuaCommand(const std::string& command);

    void onScriptPropertiesModified(entt::entity entity);

    void enterPlayMode();
    void stopPlayMode();
    void stepOnce();

    void requestExit();
    void resetLayout();

    Scene::SceneService& sceneService() const;
    Scripting::ScriptAssetService& scriptAssets() const;
    Scripting::LuaRuntimeSystem& luaRuntime() const;
    Core::PlayModeController& playMode() const;

    Rendering::Camera* activeViewportCamera();

private:
    void registerPanels();
    void synchronizeSceneScripts();
    void handleScriptAssetChange(const Assets::AssetChange& change);
    const Scripting::ScriptAsset* findScriptByName(const std::string& name) const;
    void synchronizeScriptFields(Scene::LuaScriptComponent& component, const Scripting::ScriptAsset& asset) const;

    Core::Application& m_application;
    Core::RuntimeConfig m_runtimeConfig{};
    Scripting::LuaContext* m_luaContext{nullptr};
    Rendering::Renderer* m_renderer{nullptr};
    Scene::SceneService* m_sceneService{nullptr};
    Scripting::ScriptAssetService* m_scriptAssets{nullptr};
    Scripting::LuaRuntimeSystem* m_luaRuntime{nullptr};
    Core::PlayModeController* m_playMode{nullptr};
    Scripting::LuaBindings* m_luaBindings{nullptr};

    EditorState m_state;
    std::vector<std::unique_ptr<IEditorPanel>> m_panels;

    std::unique_ptr<EditorCameraController> m_editorCamera;
    Rendering::Camera* m_runtimeCamera{nullptr};
};

}
