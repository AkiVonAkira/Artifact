#pragma once

#include "EditorState.h"
#include "Panels/PanelFactories.h"
#include "Artifact/Core/Config.h"

#include <memory>

namespace Artifact::Core {
class Application;
}

namespace Artifact::Scripting {
class LuaContext;
}

namespace Artifact::Rendering {
class Renderer;
}

namespace Artifact::Editor {

class EditorLayer {
public:
    static const ImVec4 kInfoColor;
    static const ImVec4 kWarningColor;
    static const ImVec4 kErrorColor;

    explicit EditorLayer(Artifact::Core::Application& application);

    void drawPlayControlsInMenuBar();
    void drawStatusBarContent();
    void renderSceneViewport(const ImVec2& viewportSize);

    void refreshScriptAssets();
    void createDefaultScene();

    std::string trimString(const std::string& value) const;

    EditorEntity& addEntity(const std::string& name);
    void removeSelectedEntity();
    EditorEntity* getSelectedEntity();
    void assignScriptToSelection(const std::string& scriptName);

    bool createLuaScript(const std::string& scriptName);
    void pushConsoleMessage(const std::string& message, const ImVec4& color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    void executeLuaCommand(const std::string& command);

    void enterPlayMode();
    void stopPlayMode();
    void stepOnce();
    void runEntityScripts(float deltaTime);

    void requestExit();
    void resetLayout();

private:
    void registerPanels();
    void ensureScriptsFolderExists() const;
    const LuaScriptAsset* findScriptByName(const std::string& name) const;

    Core::Application& m_application;
    Core::RuntimeConfig m_runtimeConfig{};
    Scripting::LuaContext* m_luaContext{nullptr};
    Rendering::Renderer* m_renderer{nullptr};

    EditorState m_state;
    std::vector<std::unique_ptr<IEditorPanel>> m_panels;
};

}
