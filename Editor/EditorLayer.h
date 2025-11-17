#pragma once

#include "Artifact/Core/Config.h"

#include <filesystem>
#include <string>
#include <vector>

#include <glm/vec3.hpp>
#include <imgui.h>

namespace Artifact::Core {
class Application;
}

namespace Artifact::Scripting {
class LuaContext;
}

namespace Artifact::Editor {

struct LuaScriptAsset {
    std::string name;
    std::filesystem::path path;
};

struct Transform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

struct EditorEntity {
    int id{0};
    std::string name{
        "GameObject"
    };
    bool active{true};
    Transform transform{};
    std::string scriptName{};
    bool autoReloadScript{true};
};

struct ConsoleMessage {
    std::string text;
    ImVec4 color;
};

enum class PlayMode {
    Edit = 0,
    Playing,
    Paused
};

class EditorLayer {
public:
    explicit EditorLayer(Artifact::Core::Application& application);

private:
    void registerPanels();
    void drawMainMenuBar();
    void drawToolbar();
    void drawHierarchy();
    void drawInspector();
    void drawSceneView();
    void drawConsole();
    void drawAssets();

    void refreshScriptAssets();
    void ensureScriptsFolderExists() const;
    void createDefaultScene();
    EditorEntity& addEntity(const std::string& name);
    void removeSelectedEntity();
    EditorEntity* getSelectedEntity();
    const LuaScriptAsset* findScriptByName(const std::string& name) const;
    void assignScriptToSelection(const std::string& scriptName);

    void pushConsoleMessage(const std::string& message, const ImVec4& color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    void executeLuaCommand(const std::string& command);
    void enterPlayMode();
    void stopPlayMode();
    void stepOnce();
    void runEntityScripts();

    Artifact::Core::Application& m_application;
    Artifact::Core::RuntimeConfig m_runtimeConfig{};
    Artifact::Scripting::LuaContext* m_luaContext{nullptr};

    std::vector<EditorEntity> m_entities;
    int m_selectedEntity{-1};
    int m_nextEntityId{1};
    std::vector<LuaScriptAsset> m_scriptAssets;

    PlayMode m_playMode{PlayMode::Edit};
    bool m_isPaused{false};
    float m_sceneClock{0.0f};
    std::string m_sceneName{"Untitled Scene"};

    std::vector<ConsoleMessage> m_consoleMessages;
    bool m_consoleAutoScroll{true};
    ImGuiTextFilter m_consoleFilter;
    std::string m_luaInputBuffer{};

    bool m_showDemoWindow{false};
};

}
