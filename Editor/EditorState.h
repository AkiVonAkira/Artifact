#pragma once

#include <optional>
#include <string>
#include <vector>

#include <imgui.h>

namespace Artifact::Editor {

struct ConsoleMessage {
    std::string text;
    ImVec4 color;
};

struct EditorState {
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
