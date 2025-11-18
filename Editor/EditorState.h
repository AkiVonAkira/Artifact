#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <entt/entt.hpp>
#include <imgui.h>

#include "Artifact/Scene/Components.h"
#include "Artifact/Scene/SceneContext.h"

namespace Artifact::Editor {

struct LuaScriptAsset {
    std::string name;
    std::filesystem::path path;
};

using Artifact::Scene::ActiveComponent;
using Artifact::Scene::IdComponent;
using Artifact::Scene::LuaScriptComponent;
using Artifact::Scene::NameComponent;
using Artifact::Scene::Transform;
using Artifact::Scene::TransformComponent;

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
    Scene::SceneContext scene;
    entt::entity selectedEntity{entt::null};
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
