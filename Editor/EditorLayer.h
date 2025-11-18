#pragma once

#include "EditorState.h"
#include "Panels/PanelFactories.h"
#include "Artifact/Core/Config.h"
#include "Artifact/Scripting/LuaBindings.h"

#include <memory>
#include <unordered_map>

#include <sol/sol.hpp>

namespace Artifact::Core {
class Application;
}

namespace Artifact::Assets {
struct AssetChange;
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
    void runEntityScripts(float deltaTime);

    void requestExit();
    void resetLayout();

private:
    void registerPanels();
    void ensureScriptsFolderExists() const;
    const LuaScriptAsset* findScriptByName(const std::string& name) const;
    std::vector<LuaScriptPropertyDefinition> parseScriptProperties(const std::filesystem::path& path) const;
    void synchronizeScriptFields(LuaScriptComponent& component, const LuaScriptAsset& asset) const;
    void initializeScriptComponent(entt::entity entity);
    void handleScriptAssetChange(const Assets::AssetChange& change);
    void destroyRuntime(entt::entity entity);
    struct ScriptRuntimeState {
        std::string scriptName;
        sol::environment environment;
        sol::table definition;
        sol::table instance;
        sol::table properties;
        sol::protected_function awake;
        sol::protected_function start;
        sol::protected_function update;
        sol::protected_function onEnable;
        sol::protected_function onDisable;
        bool awakeCalled{false};
        bool startCalled{false};
        bool enabledLastFrame{false};
    };
    ScriptRuntimeState* findRuntime(entt::entity entity);
    ScriptRuntimeState* getOrCreateRuntime(entt::entity entity, LuaScriptComponent& component);
    void applyPropertiesToRuntime(ScriptRuntimeState& runtime, const LuaScriptComponent& component, const LuaScriptAsset& asset);

    Core::Application& m_application;
    Core::RuntimeConfig m_runtimeConfig{};
    Scripting::LuaContext* m_luaContext{nullptr};
    Rendering::Renderer* m_renderer{nullptr};

    EditorState m_state;
    std::unique_ptr<Scripting::LuaBindings> m_luaBindings;
    std::vector<std::unique_ptr<IEditorPanel>> m_panels;
    std::unordered_map<entt::entity, ScriptRuntimeState> m_scriptRuntimes;
};

}
