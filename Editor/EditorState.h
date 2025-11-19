#pragma once

#include <optional>
#include <string>
#include <vector>

#include <imgui.h>
#include <glm/glm.hpp>

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

    struct EditorCameraState {
        glm::vec3 position{0.0f, 5.0f, 10.0f};
        glm::vec3 eulerDegrees{-20.0f, -35.0f, 0.0f};
        float moveSpeed{5.0f};
        float lookSensitivity{0.1f};
        bool orbitMode{false};
    } editorCamera;
};

}
