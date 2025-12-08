#pragma once

#include <memory>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Artifact::Rendering {
    class Camera;
}

namespace Artifact::Editor {
    struct EditorState;

    class EditorCameraController {
    public:
        explicit EditorCameraController(EditorState &state);

        void setViewportSize(const glm::vec2 &size);

        void update(float deltaSeconds, bool viewportFocused, bool viewportHovered);

        Rendering::Camera &camera();

        const Rendering::Camera &camera() const;

        void focus(const glm::vec3 &target, float distance = 5.0f);

    private:
        void applyOrbitControls(float deltaSeconds, const glm::vec2 &delta);

        void applyFlyControls(float deltaSeconds, const glm::vec2 &delta);

        EditorState &m_state;
        std::unique_ptr<Rendering::Camera> m_camera;
        glm::vec2 m_viewportSize{1.0f};
        bool m_viewportFocused{false};
        bool m_viewportHovered{false};
    };
}
