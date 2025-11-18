#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/vec3.hpp>
#include <imgui.h>

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
    std::string name{ "GameObject" };
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

struct EditorState {
    std::vector<EditorEntity> entities;
    int selectedEntity{-1};
    int nextEntityId{1};
    std::vector<LuaScriptAsset> scriptAssets;

    PlayMode playMode{PlayMode::Edit};
    bool isPaused{false};
    float sceneClock{0.0f};
    std::string sceneName{"Untitled Scene"};
    ImVec2 sceneViewportSize{0.0f, 0.0f};

    std::vector<ConsoleMessage> consoleMessages;
    bool consoleAutoScroll{true};
    ImGuiTextFilter consoleFilter;
    std::string luaInputBuffer{};

    std::string newScriptName{};
    std::string scriptCreationError{};

    bool showDemoWindow{false};
};

}
