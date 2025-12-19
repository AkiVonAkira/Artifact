#include "Artifact/Editor/EditorLayer.h"
#include "Artifact/Editor/EditorSceneManager.h"
#include "Artifact/Core/Application.h"
#include "Artifact/Core/LogManager.h"
#include "Artifact/Core/InputManager.h"
#include "Artifact/Core/TimeManager.h"
#include "Artifact/Rendering/RenderManager.h"
#include "Artifact/Scene/SceneManager.h"
#include "Artifact/Core/PlayModeController.h"
#include "Artifact/Scene/Components.h"
#include "Artifact/UI/Overlay.h"
#include "Viewport/EditorCameraController.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <algorithm>
#include <unordered_map>
#include <cctype>
#include <ImGuizmo.h>
#include <ImGuiFileDialog.h>

#include "Artifact/Core/Macros.h"
#include "Panels/Core/PanelRegistry.h"
#include "Panels/Core/PanelInstances.h"

namespace Artifact::Editor {
    const ImVec4 EditorLayer::kInfoColor{0.6f, 0.78f, 1.0f, 1.0f};
    const ImVec4 EditorLayer::kWarningColor{1.0f, 0.85f, 0.4f, 1.0f};
    const ImVec4 EditorLayer::kErrorColor{1.0f, 0.4f, 0.4f, 1.0f};

    namespace {
        std::string TrimString(const std::string &value) {
            const char *whitespace = " \t\n\r";
            const auto start = value.find_first_not_of(whitespace);
            if (start == std::string::npos) {
                return "";
            }
            const auto end = value.find_last_not_of(whitespace);
            return value.substr(start, end - start + 1);
        }

        bool IsValidScriptName(const std::string &name) {
            if (name.empty()) {
                return false;
            }
            for (unsigned char c: name) {
                if (!(std::isalnum(c) || c == '_')) {
                    return false;
                }
            }
            return true;
        }
    } // namespace


    EditorLayer::EditorLayer(Core::Application &application)
        : m_application(application)
          , m_runtimeConfig(application.runtimeConfig()) {
        LOG_INFO("EditorLayer: Starting initialization...");

        m_playMode = application.playMode();

        LOG_INFO("EditorLayer: Creating managers...");
        m_sceneManager = std::make_unique<EditorSceneManager>();
        m_viewportManager = std::make_unique<EditorViewportManager>(m_state);

        LOG_INFO("EditorLayer: Creating default scene...");
        createDefaultScene();

        LOG_INFO("EditorLayer: Registering panels...");
        registerPanels();

        LOG_INFO("EditorLayer: Setting status bar callback...");
        m_application.setStatusBarCallback([this]() {
            drawStatusBarContent();
        });

        // Now that the editor is fully initialized, enable info-level logs to show runtime info
        Core::LogManager::Instance().SetLevel(spdlog::level::info);

        LOG_INFO("GLHF!!");
    }

    EditorLayer::~EditorLayer() {
        // Cleanup handled automatically by singleton managers
    }

    void EditorLayer::registerPanels() {
        m_panelManager.removeClosedInstances();
        const auto &registry = GetPanelRegistry();
        for (const auto &descriptor: registry) {
            if (!descriptor.defaultOpen) {
                continue;
            }
            bool exists = false;
            for (const auto &instancePtr: m_panelManager.instances()) {
                if (instancePtr && instancePtr->descriptor == &descriptor) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                m_panelManager.createInstance(descriptor);
            }
        }
        rebuildPanelDescriptors();
    }

    void EditorLayer::rebuildPanelDescriptors() {
        auto *overlay = m_application.overlay();
        if (!overlay) {
            return;
        }

        m_panelManager.removeClosedInstances();

        std::vector<UI::OverlayPanelDescriptor> overlayDescriptors;
        auto &instances = m_panelManager.instances();
        for (auto &instancePtr: instances) {
            PanelInstance *panelInstance = instancePtr.get();
            if (!panelInstance || !panelInstance->panel) {
                continue;
            }
            const auto *descriptor = panelInstance->descriptor;
            bool *openFlag = descriptor && descriptor->closeable ? &panelInstance->open : nullptr;
            UI::OverlayPanelDescriptor descriptorEntry{};
            descriptorEntry.id = panelInstance->instanceId;
            descriptorEntry.label = descriptor ? descriptor->label : panelInstance->instanceId;
            descriptorEntry.allowMultiple = descriptor ? descriptor->allowMultiple : true;
            descriptorEntry.hasWindow = descriptor ? descriptor->hasWindow : true;
            descriptorEntry.closeable = descriptor ? descriptor->closeable : true;
            descriptorEntry.openFlag = openFlag;
            descriptorEntry.draw = [panelInstance, descriptor, openFlag, this]() {
                PanelContext ctx{m_state, *this, *m_playMode};
                ctx.windowTitle = descriptor ? descriptor->label.c_str() : panelInstance->instanceId.c_str();
                ctx.windowId = panelInstance->instanceId.c_str();
                ctx.openFlag = openFlag;
                ctx.hasWindow = descriptor ? descriptor->hasWindow : true;
                ctx.closeable = descriptor ? descriptor->closeable : true;
                panelInstance->panel->draw(ctx);
            };
            overlayDescriptors.push_back(std::move(descriptorEntry));
        }
        overlay->setPanelDescriptors(std::move(overlayDescriptors));
    }

    const std::vector<PanelDescriptor> &EditorLayer::panelDescriptors() const {
        return GetPanelRegistry();
    }

    void EditorLayer::openPanel(const std::string &id, bool forceNewInstance) {
        const auto *descriptor = FindPanelDescriptor(id);
        if (!descriptor) {
            return;
        }
        if (!descriptor->allowMultiple && !forceNewInstance) {
            auto &instances = m_panelManager.instances();
            for (auto &instancePtr: instances) {
                PanelInstance *panelInstance = instancePtr.get();
                if (panelInstance && panelInstance->descriptor == descriptor) {
                    panelInstance->open = true;
                    rebuildPanelDescriptors();
                    return;
                }
            }
        }
        if (m_panelManager.createInstance(*descriptor)) {
            rebuildPanelDescriptors();
        }
    }

    Core::PlayModeController &EditorLayer::playMode() const {
        return *m_playMode;
    }

    void EditorLayer::drawPlayControlsInMenuBar() {
        const float buttonWidth = 90.0f;
        const ImGuiStyle &style = ImGui::GetStyle();
        const float spacing = style.ItemSpacing.x;
        const float buttonBlockWidth = buttonWidth * 3.0f + spacing * 2.0f;

        const float availableWidth = ImGui::GetWindowWidth();
        const float desiredStart = (availableWidth - buttonBlockWidth) * 0.5f;
        float startX = std::max(desiredStart, ImGui::GetCursorPosX());

        ImGui::SameLine(0.0f, spacing);
        ImGui::SetCursorPosX(startX);

        const bool isPlaying = m_playMode && m_playMode->state() == Core::PlayModeController::State::Playing;
        const bool isPaused = m_playMode && m_playMode->paused();
        const ImVec4 playColor = isPlaying ? ImVec4(0.2f, 0.8f, 0.3f, 1.0f) : style.Colors[ImGuiCol_Button];
        const ImVec4 pauseColor = isPaused ? ImVec4(0.95f, 0.8f, 0.2f, 1.0f) : style.Colors[ImGuiCol_Button];

        ImGui::PushStyleColor(ImGuiCol_Button, playColor);
        if (ImGui::Button(isPlaying ? "Stop" : "Play", ImVec2(buttonWidth, 0.0f))) {
            if (isPlaying) {
                stopPlayMode();
            } else {
                enterPlayMode();
            }
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, pauseColor);
        if (ImGui::Button("Pause", ImVec2(buttonWidth, 0.0f))) {
            if (m_playMode && isPlaying) {
                const bool nextPause = !m_playMode->paused();
                m_playMode->pause(nextPause);
                pushConsoleMessage(nextPause ? "Gameplay paused" : "Gameplay resumed");
            }
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();

        if (ImGui::Button("Step", ImVec2(buttonWidth, 0.0f))) {
            stepOnce();
        }
    }

    void EditorLayer::createDefaultScene() {
        if (!m_sceneManager) {
            return;
        }
        // Create a default camera entity
        auto& registry = m_sceneManager->sceneContext().registry;
        auto camera = registry.create();
        registry.emplace<Scene::NameComponent>(camera, "Main Camera");
        registry.emplace<Scene::ActiveComponent>(camera, true);
        auto& transform = registry.emplace<Scene::TransformComponent>(camera);
        transform.transform.position = glm::vec3(-5.0f, 2.5f, 0.0f);
        transform.transform.rotation = glm::vec3(-25.0f, -90.0f, 0.0f);
        auto& cameraComp = registry.emplace<Scene::CameraComponent>(camera);
        cameraComp.isPrimary = true;
        cameraComp.enabled = true;

        // Create a default mesh entity
        auto meshEntity = registry.create();
        registry.emplace<Scene::NameComponent>(meshEntity, "Viking House");
        registry.emplace<Scene::ActiveComponent>(meshEntity, true);
        auto& meshTransform = registry.emplace<Scene::TransformComponent>(meshEntity);
        meshTransform.transform.scale = glm::vec3(10.0f, 10.0f, 10.0f);

        auto& meshComp = registry.emplace<Scene::MeshComponent>(meshEntity);
        meshComp.meshPath = "Assets/Models/Viking_House.obj";

        auto& materialComp = registry.emplace<Scene::MaterialComponent>(meshEntity);
        materialComp.albedoTexture = "Assets/Models/Viking_House.png";
        materialComp.color = glm::vec4(1.0f);
    }

    // Entity management methods - delegate to EditorSceneManager
    entt::entity EditorLayer::addEntity(const std::string& name) {
        return m_sceneManager ? m_sceneManager->addEntity(name) : entt::null;
    }

    void EditorLayer::duplicateEntity(entt::entity entity) {
        if (m_sceneManager) {
            m_sceneManager->duplicateEntity(entity);
        }
    }

    void EditorLayer::removeSelectedEntity() {
        if (m_sceneManager) {
            m_sceneManager->removeSelectedEntity();
        }
    }

    void EditorLayer::selectEntity(entt::entity entity) {
        if (m_sceneManager) {
            m_sceneManager->selectEntity(entity);
        }
    }

    entt::entity EditorLayer::getSelectedEntity() const {
        return m_sceneManager ? m_sceneManager->getSelectedEntity() : entt::null;
    }

    void EditorLayer::pushConsoleMessage(const std::string &message, spdlog::level::level_enum level) {
        spdlog::log(level, "{}", message);
    }

    void EditorLayer::enterPlayMode() {
        if (!m_playMode) {
            return;
        }
        m_playMode->play();
        m_state.sceneClock = 0.0f;
        if (m_sceneManager) {
            // Create a runtime copy of the scene so runtime changes don't persist
            // TODO: Implement proper scene snapshot
            m_runtimeActive = true;
            m_runtimeCamera = Rendering::RenderManager::Instance().BindRuntimeCamera(m_runtimeScene.registry);
        }
        pushConsoleMessage("Entering play mode");
    }

    void EditorLayer::stopPlayMode() {
        if (!m_playMode) {
            return;
        }
        m_playMode->stop();
        m_state.sceneClock = 0.0f;
        m_runtimeCamera = nullptr;
        m_runtimeActive = false;
        m_runtimeScene = Scene::SceneContext();
        pushConsoleMessage("Exited play mode");
    }

    void EditorLayer::stepOnce() {
        if (!m_playMode) {
            return;
        }
        if (m_playMode->state() != Core::PlayModeController::State::Playing) {
            m_playMode->play();
        }
        m_playMode->step();
        pushConsoleMessage("Stepped one frame");
        // Script tick removed - no longer using Lua scripting
        // if (m_runtimeActive) {
        //     Scripting::ScriptManager::Instance().Tick(m_runtimeScene.registry, 1.0f / 60.0f, true);
        // } else {
        //     Scripting::ScriptManager::Instance().Tick(m_sceneService->context().registry, 1.0f / 60.0f, true);
        // }
    }

    void EditorLayer::requestExit() {
        m_application.requestExit();
    }

    void EditorLayer::resetLayout() {
        m_application.resetLayout();
    }

    void EditorLayer::drawStatusBarContent() {
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Scene: %s", m_state.sceneName.c_str());
    }

    std::string EditorLayer::trimString(const std::string &value) const {
        return TrimString(value);
    }

    std::string EditorLayer::requestMeshAsset(const std::string &currentPath) {
        std::string selectedPath = currentPath;
        IGFD::FileDialogConfig config;
        config.path = currentPath;
        config.fileName = currentPath;
        config.flags = ImGuiFileDialogFlags_Default;
        ImGuiFileDialog::Instance()->OpenDialog("MeshAssetDialog", "Select Mesh", "*.obj,*.fbx,*.glb", config);
        if (ImGuiFileDialog::Instance()->Display("MeshAssetDialog")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                selectedPath = ImGuiFileDialog::Instance()->GetFilePathName();
            }
            ImGuiFileDialog::Instance()->Close();
        }
        return selectedPath;
    }

    std::string EditorLayer::requestTextureAsset(const std::string &currentPath) {
        std::string selectedPath = currentPath;
        IGFD::FileDialogConfig config;
        config.path = currentPath;
        config.fileName = currentPath;
        config.flags = ImGuiFileDialogFlags_Default;
        ImGuiFileDialog::Instance()->OpenDialog("TextureAssetDialog", "Select Texture", "*.png,*.jpg,*.jpeg,*.bmp,*.tga,*.hdr", config);
        if (ImGuiFileDialog::Instance()->Display("TextureAssetDialog")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                selectedPath = ImGuiFileDialog::Instance()->GetFilePathName();
            }
            ImGuiFileDialog::Instance()->Close();
        }
        return selectedPath;
    }
} // namespace Artifact::Editor
