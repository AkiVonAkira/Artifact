#include "Artifact/Editor/EditorViewportManager.h"
#include "Artifact/Scene/SceneService.h"
#include "Artifact/Scene/SceneContext.h"
#include "Artifact/Scene/Components.h"
#include "Artifact/Rendering/RenderManager.h"
#include "Artifact/Rendering/Camera.h"
#include "Artifact/Core/PlayModeController.h"
#include <glm/gtc/quaternion.hpp>
#include <algorithm>

#include "Viewport/EditorCameraController.h"

struct ImVec2;

namespace Artifact::Editor {
    struct EditorState;

    EditorViewportManager::EditorViewportManager(EditorState& state)
        : m_state(state) {
        m_editorCamera = std::make_unique<EditorCameraController>(m_state);
    }

    void EditorViewportManager::renderSceneViewport(
        const ImVec2& viewportSize,
        Scene::SceneService* sceneService,
        Core::PlayModeController* playMode,
        const Scene::SceneContext* runtimeScene,
        bool runtimeActive
    ) {
        m_state.sceneViewportSize = viewportSize;
        const bool viewportFocused = ImGui::IsWindowFocused();
        const bool viewportHovered = ImGui::IsWindowHovered();

        if (m_editorCamera) {
            m_editorCamera->setViewportSize({viewportSize.x, viewportSize.y});
            m_editorCamera->update(ImGui::GetIO().DeltaTime, viewportFocused, viewportHovered);
        }

        if (viewportSize.x <= 0.0f || viewportSize.y <= 0.0f) {
            ImGui::TextUnformatted("Scene viewport unavailable");
            return;
        }

        // Update scene clock if playing
        if (playMode && playMode->state() == Core::PlayModeController::State::Playing) {
            const float deltaTime = ImGui::GetIO().DeltaTime;
            if (!playMode->paused()) {
                m_state.sceneClock += deltaTime;
            }
        }

        // Determine active camera
        Rendering::Camera* camera = m_editorCamera ? &m_editorCamera->camera() : nullptr;
        if (playMode && playMode->state() == Core::PlayModeController::State::Playing) {
            if (runtimeScene) {
                camera = Rendering::RenderManager::Instance().BindRuntimeCamera(
                    const_cast<entt::registry&>(runtimeScene->registry)
                );
            }
        }

        // Set skybox colors
        const std::array<float, 3> top{
            m_state.skybox.topColor.x,
            m_state.skybox.topColor.y,
            m_state.skybox.topColor.z
        };
        const std::array<float, 3> horizon{
            m_state.skybox.horizonColor.x,
            m_state.skybox.horizonColor.y,
            m_state.skybox.horizonColor.z
        };
        Rendering::RenderManager::Instance().SetSkyboxColors(top, horizon);

        // Render scene
        Rendering::RenderManager::Instance().BeginSceneViewport(
            static_cast<int>(viewportSize.x),
            static_cast<int>(viewportSize.y)
        );

        if (sceneService) {
            if (runtimeActive && runtimeScene) {
                Rendering::RenderManager::Instance().RenderScene(*runtimeScene, camera);
            } else {
                Rendering::RenderManager::Instance().RenderScene(sceneService->context(), camera);
            }
        } else {
            Rendering::RenderManager::Instance().DrawSkybox();
        }

        Rendering::RenderManager::Instance().EndSceneViewport();

        // Display rendered texture
        const uint32_t texture = Rendering::RenderManager::Instance().SceneColorTexture();
        if (texture != 0) {
            ImGui::Image(
                reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(texture)),
                viewportSize,
                ImVec2(0, 1),
                ImVec2(1, 0)
            );

            drawEditorCameraOverlay(viewportSize, viewportFocused, viewportHovered);
            drawCameraPreviewOverlay(viewportSize, sceneService);
        }
    }

    void EditorViewportManager::drawEditorCameraOverlay(
        const ImVec2& viewportSize,
        bool viewportFocused,
        bool viewportHovered
    ) {
        if (!m_editorCamera) {
            return;
        }

        const ImVec2 overlayPadding(16.0f, 16.0f);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 panelMin = ImGui::GetItemRectMin();
        const ImVec2 panelMax = ImGui::GetItemRectMax();
        const ImVec2 panelSize = {panelMax.x - panelMin.x, panelMax.y - panelMin.y};

        const auto& cam = m_editorCamera->camera();
        const glm::vec3 position = cam.position();
        const glm::vec3 euler = cam.eulerAngles();

        char buffer[256];
        std::snprintf(buffer, sizeof(buffer),
            "Cam Pos: (%.2f, %.2f, %.2f)\nRot: (%.1f, %.1f, %.1f)",
            position.x, position.y, position.z,
            euler.x, euler.y, euler.z
        );

        const ImVec2 textSize = ImGui::CalcTextSize(buffer);
        const ImVec2 backgroundPos = {panelMin.x + overlayPadding.x, panelMin.y + overlayPadding.y};
        const ImVec2 backgroundEnd = {
            backgroundPos.x + textSize.x + 12.0f,
            backgroundPos.y + textSize.y + 52.0f
        };

        drawList->AddRectFilled(backgroundPos, backgroundEnd, IM_COL32(22, 22, 22, 200), 6.0f);
        drawList->AddRect(backgroundPos, backgroundEnd, IM_COL32(60, 60, 60, 200), 6.0f);
        drawList->AddText(
            {backgroundPos.x + 6.0f, backgroundPos.y + 6.0f},
            IM_COL32_WHITE,
            buffer
        );

        ImGui::SetCursorScreenPos({backgroundPos.x + 6.0f, backgroundPos.y + textSize.y + 12.0f});
        ImGui::PushItemWidth(140.0f);
        ImGui::SliderFloat("Frustum Depth", &m_state.editorCamera.frustumPreviewDepth, 5.0f, 200.0f, "%.0f");
        ImGui::PopItemWidth();

        drawViewportInputHints(panelMin, panelSize, viewportFocused, viewportHovered);
    }

    void EditorViewportManager::drawViewportInputHints(
        const ImVec2& panelMin,
        const ImVec2& panelSize,
        bool focused,
        bool hovered
    ) {
        const char* hint = focused || hovered
            ? "WASD/EQ = Fly | Right-drag = Look | Middle-drag = Orbit"
            : "Click viewport to control camera";

        const ImVec2 textSize = ImGui::CalcTextSize(hint);
        const float paddingX = 16.0f;
        const float paddingY = 10.0f;
        const ImVec2 boxMin = {
            panelMin.x + paddingX,
            panelMin.y + panelSize.y - textSize.y - paddingY
        };
        const ImVec2 boxMax = {
            boxMin.x + textSize.x + paddingX,
            boxMin.y + textSize.y + paddingY
        };

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(boxMin, boxMax, IM_COL32(20, 20, 20, 160), 6.0f);
        drawList->AddRect(boxMin, boxMax, IM_COL32(70, 70, 70, 200), 6.0f);
        drawList->AddText(
            {boxMin.x + paddingX * 0.5f, boxMin.y + paddingY * 0.3f},
            IM_COL32(220, 220, 220, 255),
            hint
        );
    }

    void EditorViewportManager::drawSceneCameraFrustums(
        const ImVec2& panelMin,
        const ImVec2& panelSize,
        Scene::SceneService* sceneService
    ) {
        if (!sceneService || !m_editorCamera) {
            return;
        }

        auto& registry = sceneService->context().registry;
        auto view = registry.view<Scene::TransformComponent, Scene::CameraComponent>();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float previewDepth = std::max(1.0f, m_state.editorCamera.frustumPreviewDepth);

        view.each([&](entt::entity, Scene::TransformComponent& transform, Scene::CameraComponent& cameraComp) {
            if (!cameraComp.enabled) {
                return;
            }

            const glm::vec3 positionWorld = transform.transform.position;
            const glm::quat cameraRotation = glm::quat(glm::radians(transform.transform.rotation));
            const glm::vec3 cameraForward = glm::normalize(cameraRotation * glm::vec3(0.0f, 0.0f, -1.0f));
            const glm::vec3 cameraRight = glm::normalize(cameraRotation * glm::vec3(1.0f, 0.0f, 0.0f));
            const glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, cameraForward));

            const float farDistance = std::min(cameraComp.camera.farPlane(), previewDepth);
            const float halfVertical = glm::tan(glm::radians(cameraComp.camera.fieldOfView() * 0.5f)) * farDistance;
            const float halfHorizontal = halfVertical * cameraComp.camera.aspectRatio();
            const glm::vec3 farCenter = positionWorld + cameraForward * farDistance;

            const glm::vec3 corners[4] = {
                farCenter + cameraRight * halfHorizontal + cameraUp * halfVertical,
                farCenter - cameraRight * halfHorizontal + cameraUp * halfVertical,
                farCenter - cameraRight * halfHorizontal - cameraUp * halfVertical,
                farCenter + cameraRight * halfHorizontal - cameraUp * halfVertical
            };

            // Project camera position
            const glm::vec2 screenPoint = m_editorCamera->camera().worldToScreenPoint(
                positionWorld, {panelSize.x, panelSize.y}
            );
            if (!std::isfinite(screenPoint.x) || !std::isfinite(screenPoint.y)) {
                return;
            }

            const ImVec2 origin = {panelMin.x + screenPoint.x, panelMin.y + screenPoint.y};
            const float size = 14.0f;

            drawList->AddCircleFilled(origin, size * 0.35f, IM_COL32(255, 255, 255, 180));
            drawList->AddText({origin.x + size * 0.5f, origin.y - size}, IM_COL32(255, 255, 0, 230), "Camera");

            // Draw lines from camera to frustum corners
            for (const glm::vec3& corner : corners) {
                const glm::vec2 projectedCorner = m_editorCamera->camera().worldToScreenPoint(
                    corner, {panelSize.x, panelSize.y}
                );
                if (!std::isfinite(projectedCorner.x) || !std::isfinite(projectedCorner.y)) {
                    continue;
                }
                const ImVec2 cornerPos = {panelMin.x + projectedCorner.x, panelMin.y + projectedCorner.y};
                drawList->AddLine(origin, cornerPos, IM_COL32(200, 200, 0, 200), 1.0f);
            }

            // Draw frustum edges
            for (int i = 0; i < 4; ++i) {
                const glm::vec2 a = m_editorCamera->camera().worldToScreenPoint(
                    corners[i], {panelSize.x, panelSize.y}
                );
                const glm::vec2 b = m_editorCamera->camera().worldToScreenPoint(
                    corners[(i + 1) % 4], {panelSize.x, panelSize.y}
                );
                if (!std::isfinite(a.x) || !std::isfinite(a.y) || !std::isfinite(b.x) || !std::isfinite(b.y)) {
                    continue;
                }
                drawList->AddLine(
                    {panelMin.x + a.x, panelMin.y + a.y},
                    {panelMin.x + b.x, panelMin.y + b.y},
                    IM_COL32(180, 180, 0, 180),
                    1.0f
                );
            }
        });
    }

    void EditorViewportManager::drawCameraPreviewOverlay(
        const ImVec2& viewportSize,
        Scene::SceneService* sceneService
    ) {
        if (!sceneService || !m_state.cameraPreview.showPreview) {
            return;
        }

        auto& registry = sceneService->context().registry;
        auto view = registry.view<Scene::CameraComponent>();
        Scene::CameraComponent* primaryCam = nullptr;

        view.each([&](entt::entity, Scene::CameraComponent& cameraComp) {
            if (!cameraComp.enabled) {
                return;
            }
            if (!primaryCam || cameraComp.isPrimary) {
                primaryCam = &cameraComp;
            }
        });

        if (!primaryCam) {
            return;
        }

        const float previewScale = m_state.cameraPreview.previewScale;
        const ImVec2 previewSize = {viewportSize.x * previewScale, viewportSize.y * previewScale};

        const uint32_t previewTexture = Rendering::RenderManager::Instance().RenderScenePreview(
            sceneService->context(),
            static_cast<int>(previewSize.x),
            static_cast<int>(previewSize.y),
            &primaryCam->camera
        );

        if (previewTexture == 0) {
            return;
        }

        ImGui::SetCursorPos({
            viewportSize.x - previewSize.x - m_state.cameraPreview.padding,
            viewportSize.y - previewSize.y - m_state.cameraPreview.padding
        });

        ImGui::BeginChild("CameraPreviewOverlay", previewSize, true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );

        ImGui::TextUnformatted("Camera Preview");
        ImGui::Separator();

        const ImTextureID previewTextureId = reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(previewTexture));
        ImGui::Image(previewTextureId, previewSize, ImVec2(0, 1), ImVec2(1, 0));

        ImGui::Separator();
        ImGui::Text("FOV: %.1f", primaryCam->camera.fieldOfView());
        ImGui::Text("Near/Far: %.2f / %.2f", primaryCam->camera.nearPlane(), primaryCam->camera.farPlane());
        ImGui::Text("Primary: %s", primaryCam->isPrimary ? "Yes" : "No");

        ImGui::EndChild();
    }

    Rendering::Camera* EditorViewportManager::activeViewportCamera(
        Core::PlayModeController* playMode,
        Rendering::Camera* runtimeCamera
    ) {
        if (playMode && playMode->state() == Core::PlayModeController::State::Playing) {
            return runtimeCamera;
        }
        return m_editorCamera ? &m_editorCamera->camera() : nullptr;
    }

} // namespace Artifact::Editor

