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
            bool showFrustum{true};
            float frustumPreviewDepth{50.0f};
        } editorCamera;

        struct SkyboxSettings {
            glm::vec3 topColor{0.21f, 0.33f, 0.52f};
            glm::vec3 horizonColor{0.74f, 0.80f, 0.86f};
            bool enableSunDisk{false};
            float horizonHeight{0.35f};
        } skybox;

        struct CameraPreviewSettings {
            bool showPreview{true};
            float previewScale{0.3f};
            float padding{16.0f};
            glm::vec2 lastViewportSize{0.0f};
        } cameraPreview;
    };
}
