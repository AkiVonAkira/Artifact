#include "EditorCameraController.h"
#include <imgui.h>
#include <algorithm>
#include "Artifact/Rendering/Camera.h"
#include "Artifact/Editor/EditorState.h"
#include "Artifact/Core/InputManager.h"

Artifact::Editor::EditorCameraController::EditorCameraController(EditorState &state)
    : m_state(state)
      , m_camera(std::make_unique<Rendering::Camera>(Rendering::ProjectionType::Perspective)) {
    m_camera->setPosition(m_state.editorCamera.position);

    // Initialize pitch/yaw from state
    glm::vec3 euler = m_state.editorCamera.eulerDegrees;
    m_pitch = euler.x;
    m_yaw = euler.y;
    updateCameraRotation();
}

void Artifact::Editor::EditorCameraController::setViewportSize(const glm::vec2 &size) {
    m_viewportSize = size;
    const float aspect = size.y <= 0.0f ? 1.0f : size.x / size.y;
    m_camera->setPerspective(m_camera->fieldOfView(), aspect, m_camera->nearPlane(), m_camera->farPlane());
}

void Artifact::Editor::EditorCameraController::update(float deltaSeconds, bool viewportFocused, bool viewportHovered) {
    m_viewportFocused = viewportFocused;
    m_viewportHovered = viewportHovered;

    if (!viewportFocused && !viewportHovered) {
        return;
    }

    // Use InputManager mouse delta instead of ImGui's to avoid capture snags
    double mdx = 0.0, mdy = 0.0;
    Artifact::Core::InputManager::Instance().GetMouseDelta(mdx, mdy);
    const glm::vec2 delta{static_cast<float>(mdx), static_cast<float>(mdy)};

    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        applyFlyControls(deltaSeconds, delta);
    } else if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        applyOrbitControls(deltaSeconds, delta);
    }

    m_state.editorCamera.position = m_camera->position();
    m_state.editorCamera.eulerDegrees = glm::vec3(m_pitch, m_yaw, 0.0f);

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const ImVec2 center{(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f};
    const float size = 10.0f;
    drawList->AddLine({center.x - size, center.y}, {center.x + size, center.y}, IM_COL32(255, 255, 255, 120), 2.0f);
    drawList->AddLine({center.x, center.y - size}, {center.x, center.y + size}, IM_COL32(255, 255, 255, 120), 2.0f);
}

Artifact::Rendering::Camera &Artifact::Editor::EditorCameraController::camera() {
    return *m_camera;
}

const Artifact::Rendering::Camera &Artifact::Editor::EditorCameraController::camera() const {
    return *m_camera;
}

void Artifact::Editor::EditorCameraController::focus(const glm::vec3 &target, float distance) {
    // Calculate direction from target to current position to maintain angle if possible,
    // or just back up along forward vector
    glm::vec3 direction = -m_camera->forward();
    m_camera->setPosition(target + direction * distance);
    m_camera->lookAt(target, glm::vec3(0.0f, 1.0f, 0.0f));

    // Update pitch/yaw from new rotation
    glm::vec3 euler = m_camera->eulerAngles();
    m_pitch = euler.x;
    m_yaw = euler.y;
}

void Artifact::Editor::EditorCameraController::applyOrbitControls(float deltaSeconds, const glm::vec2 &delta) {
    const float sensitivity = m_state.editorCamera.lookSensitivity;

    m_yaw -= delta.x * sensitivity;
    m_pitch -= delta.y * sensitivity;
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

    updateCameraRotation();

    // Orbit logic: move camera around a pivot point
    // For now, let's just rotate in place as the original code seemed to do (it was just rotation)
    // If we want true orbit, we need a pivot.
    // The original code:
    // euler.x -= delta.y * sensitivity;
    // euler.y -= delta.x * sensitivity;
    // m_camera->setRotation(euler);
    // This is just rotation, not orbiting around a point.
    // "Middle-drag = Orbit" usually implies moving the camera position too.
    // But for now, let's stick to fixing the rotation stability.
    // If the user wants true orbit, we need a target.

    // Let's implement a simple "pan" for middle mouse if shift is held, otherwise rotate?
    // Or just rotate for now to match previous behavior but stable.
}

void Artifact::Editor::EditorCameraController::applyFlyControls(float deltaSeconds, const glm::vec2 &delta) {
    const float sensitivity = m_state.editorCamera.lookSensitivity;

    m_yaw -= delta.x * sensitivity;
    m_pitch -= delta.y * sensitivity;
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

    updateCameraRotation();

    glm::vec3 move{0.0f};
    auto &io = ImGui::GetIO();
    if (ImGui::IsKeyDown(ImGuiKey_W)) move += m_camera->forward();
    if (ImGui::IsKeyDown(ImGuiKey_S)) move -= m_camera->forward();
    if (ImGui::IsKeyDown(ImGuiKey_D)) move += m_camera->right();
    if (ImGui::IsKeyDown(ImGuiKey_A)) move -= m_camera->right();
    if (ImGui::IsKeyDown(ImGuiKey_E)) move += m_camera->up();
    if (ImGui::IsKeyDown(ImGuiKey_Q)) move -= m_camera->up();

    if (glm::length(move) > 0.0f) {
        move = glm::normalize(move);
        float speed = m_state.editorCamera.moveSpeed;
        if (io.KeyShift) {
            speed *= 2.0f;
        }
        m_camera->setPosition(m_camera->position() + move * speed * deltaSeconds);
    }
}

void Artifact::Editor::EditorCameraController::updateCameraRotation() {
    m_camera->setRotation(glm::vec3(m_pitch, m_yaw, 0.0f));
}
