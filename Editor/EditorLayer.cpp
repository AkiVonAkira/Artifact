#include "EditorLayer.h"

#include "Artifact/Core/Application.h"
#include "Artifact/Rendering/Renderer.h"
#include "Artifact/Scripting/LuaContext.h"
#include "Artifact/Scene/SceneService.h"
#include "Artifact/Scripting/ScriptAssetService.h"
#include "Artifact/Scripting/LuaRuntimeSystem.h"
#include "Artifact/Core/PlayModeController.h"
#include "Artifact/Scripting/LuaBindings.h"
#include "Artifact/Scene/Components.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <algorithm>
#include <unordered_map>
#include <cctype>

#include "Panels/Panel.h"

namespace Artifact::Editor {

const ImVec4 EditorLayer::kInfoColor{0.6f, 0.78f, 1.0f, 1.0f};
const ImVec4 EditorLayer::kWarningColor{1.0f, 0.85f, 0.4f, 1.0f};
const ImVec4 EditorLayer::kErrorColor{1.0f, 0.4f, 0.4f, 1.0f};

namespace {

    std::string TrimString(const std::string& value) {
        const char* whitespace = " \t\n\r";
        const auto start = value.find_first_not_of(whitespace);
        if (start == std::string::npos) {
            return "";
        }
        const auto end = value.find_last_not_of(whitespace);
        return value.substr(start, end - start + 1);
    }

    bool IsValidScriptName(const std::string& name) {
        if (name.empty()) {
            return false;
        }
        for (unsigned char c : name) {
            if (!(std::isalnum(c) || c == '_')) {
                return false;
            }
        }
        return true;
    }

    void InvokeLifecycle(const sol::protected_function& fn,
                         const sol::table& instance,
                         const char* label,
                         std::optional<float> delta = std::nullopt) {
        if (!fn.valid()) {
            return;
        }

        sol::protected_function_result result = delta.has_value() ? fn(instance, *delta) : fn(instance);
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[Lua] {} failed: {}", label, err.what());
        }
    }

} // namespace


EditorLayer::EditorLayer(Core::Application& application)
    : m_application(application)
    , m_runtimeConfig(application.runtimeConfig()) {
    m_luaContext = application.luaContext();
    m_renderer = application.renderer();
    m_sceneService = application.sceneService();
    m_scriptAssets = application.scriptAssets();
    m_luaRuntime = application.luaRuntime();
    m_playMode = application.playMode();
    m_luaBindings = application.luaBindings();

    if (m_luaBindings) {
        m_luaBindings->registerInputBindings([this](int key) {
            return m_application.isKeyDown(key);
        });
        m_luaBindings->registerTimeBindings();
        m_luaBindings->registerEntityBindings();
        if (m_sceneService) {
            m_luaBindings->setRegistry(&m_sceneService->context().registry);
        }
    }

    refreshScriptAssets();
    m_application.registerScriptAssetListener([this](const Assets::AssetChange& change) {
        handleScriptAssetChange(change);
    });
    createDefaultScene();

    registerPanels();
    m_application.setStatusBarCallback([this]() {
        drawStatusBarContent();
    });
    pushConsoleMessage("Unity-style editor ready. Happy scripting!", kInfoColor);
}

void EditorLayer::registerPanels() {
    PanelContext ctx{m_state, *this, *m_sceneService, *m_scriptAssets, *m_luaRuntime, *m_playMode};
    m_panels.emplace_back(CreateMainMenuPanel());
    m_panels.emplace_back(CreateHierarchyPanel());
    m_panels.emplace_back(CreateInspectorPanel());
    m_panels.emplace_back(CreateScenePanel());
    m_panels.emplace_back(CreateConsolePanel());
    m_panels.emplace_back(CreateAssetsPanel());

    for (auto& panel : m_panels) {
        if (!panel) {
            continue;
        }
        m_application.registerEditorPanel([panelPtr = panel.get(), this]() {
            PanelContext panelCtx{m_state, *this, *m_sceneService, *m_scriptAssets, *m_luaRuntime, *m_playMode};
            panelPtr->draw(panelCtx);
        });
    }
}

Scene::SceneService& EditorLayer::sceneService() const {
    return *m_sceneService;
}

Scripting::ScriptAssetService& EditorLayer::scriptAssets() const {
    return *m_scriptAssets;
}

Scripting::LuaRuntimeSystem& EditorLayer::luaRuntime() const {
    return *m_luaRuntime;
}

Core::PlayModeController& EditorLayer::playMode() const {
    return *m_playMode;
}

void EditorLayer::drawPlayControlsInMenuBar() {
    const float buttonWidth = 90.0f;
    const ImGuiStyle& style = ImGui::GetStyle();
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
            pushConsoleMessage(nextPause ? "Gameplay paused" : "Gameplay resumed", kInfoColor);
        }
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();

    if (ImGui::Button("Step", ImVec2(buttonWidth, 0.0f))) {
        stepOnce();
    }
}

void EditorLayer::renderSceneViewport(const ImVec2& viewportSize) {
    m_state.sceneViewportSize = viewportSize;

    if (!m_renderer || viewportSize.x <= 0.0f || viewportSize.y <= 0.0f) {
        ImGui::TextUnformatted("Scene viewport unavailable");
        return;
    }

    if (m_playMode && m_playMode->state() == Core::PlayModeController::State::Playing) {
        const float deltaTime = ImGui::GetIO().DeltaTime;
        if (!m_playMode->paused()) {
            m_state.sceneClock += deltaTime;
        }
        if (m_luaRuntime && m_sceneService) {
            const bool shouldUpdate = m_playMode->state() == Core::PlayModeController::State::Playing && !m_playMode->paused();
            m_luaRuntime->tick(m_sceneService->context().registry, deltaTime, shouldUpdate);
        }
    }

    m_renderer->beginSceneViewport(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
    m_renderer->drawSkybox();
    m_renderer->endSceneViewport();

    const uint32_t texture = m_renderer->sceneColorTexture();
    if (texture != 0) {
        ImGui::Image(texture, viewportSize, ImVec2(0, 1), ImVec2(1, 0));
    }
}

void EditorLayer::refreshScriptAssets(bool announce) {
    if (!m_scriptAssets) {
        return;
    }
    m_scriptAssets->refresh(announce);
    synchronizeSceneScripts();
}

void EditorLayer::createDefaultScene() {
    if (!m_sceneService) {
        return;
    }
    m_sceneService->createDefaultScene(m_scriptAssets);
}

entt::entity EditorLayer::addEntity(const std::string& name) {
    return m_sceneService ? m_sceneService->createEntity(name) : entt::null;
}

void EditorLayer::duplicateEntity(entt::entity entity) {
    if (!m_sceneService) {
        return;
    }
    const entt::entity duplicate = m_sceneService->duplicateEntity(entity);
    selectEntity(duplicate);
}

void EditorLayer::removeSelectedEntity() {
    if (!m_sceneService) {
        return;
    }
    m_sceneService->destroyEntity(getSelectedEntity());
}

void EditorLayer::selectEntity(entt::entity entity) {
    if (m_sceneService) {
        m_sceneService->selectEntity(entity);
    }
}

entt::entity EditorLayer::getSelectedEntity() const {
    return m_sceneService ? m_sceneService->selectedEntity() : entt::null;
}

void EditorLayer::assignScriptToSelection(const std::string& scriptName) {
    if (!m_sceneService) {
        return;
    }
    const auto selected = m_sceneService->selectedEntity();
    if (selected == entt::null) {
        return;
    }
    auto& registry = m_sceneService->context().registry;
    if (auto* script = registry.try_get<Scene::LuaScriptComponent>(selected)) {
        const bool changedScript = script->scriptName != scriptName;
        script->scriptName = scriptName;
        if (changedScript) {
            if (const auto* asset = findScriptByName(scriptName)) {
                synchronizeScriptFields(*script, *asset);
            } else {
                script->serializedFields.clear();
            }
            if (m_luaRuntime) {
                m_luaRuntime->onScriptAssigned(selected, *script);
            }
        }
        const auto* nameComponent = registry.try_get<Scene::NameComponent>(selected);
        const std::string entityName = nameComponent ? nameComponent->name : "Entity";
        pushConsoleMessage("Assigned script '" + scriptName + "' to " + entityName, kInfoColor);
    }
}

const Scripting::ScriptAsset* EditorLayer::findScriptByName(const std::string& name) const {
    return m_scriptAssets ? m_scriptAssets->findByName(name) : nullptr;
}

void EditorLayer::pushConsoleMessage(const std::string& message, const ImVec4& color) {
    m_state.consoleMessages.push_back(ConsoleMessage{message, color});
    spdlog::info("[Editor] {}", message);
}

void EditorLayer::executeLuaCommand(const std::string& command) {
    if (command.empty()) {
        return;
    }
    if (!m_luaContext) {
        pushConsoleMessage("Lua context unavailable", kErrorColor);
        return;
    }
    pushConsoleMessage("> " + command, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    m_luaContext->executeString(command);
}

void EditorLayer::enterPlayMode() {
    if (!m_playMode) {
        return;
    }
    m_playMode->play();
    m_state.sceneClock = 0.0f;
    pushConsoleMessage("Entering play mode", kInfoColor);
}

void EditorLayer::stopPlayMode() {
    if (!m_playMode) {
        return;
    }
    m_playMode->stop();
    m_state.sceneClock = 0.0f;
    pushConsoleMessage("Exited play mode", kInfoColor);
}

void EditorLayer::stepOnce() {
    if (!m_playMode || !m_sceneService || !m_luaRuntime) {
        return;
    }
    if (m_playMode->state() != Core::PlayModeController::State::Playing) {
        m_playMode->play();
    }
    m_playMode->step();
    pushConsoleMessage("Stepped one frame", kInfoColor);
    m_luaRuntime->tick(m_sceneService->context().registry, 1.0f / 60.0f, true);
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

std::string EditorLayer::trimString(const std::string& value) const {
    return TrimString(value);
}

void EditorLayer::handleScriptAssetChange(const Assets::AssetChange& change) {
    refreshScriptAssets(false);
    const std::string scriptName = change.path.stem().string();
    switch (change.type) {
        case Assets::AssetChange::Type::Added:
            pushConsoleMessage("Lua script added: " + scriptName, kInfoColor);
            break;
        case Assets::AssetChange::Type::Modified:
            pushConsoleMessage("Lua script updated: " + scriptName, kInfoColor);
            break;
        case Assets::AssetChange::Type::Moved: {
            const std::string oldName = change.previousPath.empty() ? std::string{} : change.previousPath.stem().string();
            if (!oldName.empty() && oldName != scriptName) {
                pushConsoleMessage("Lua script renamed: " + oldName + " -> " + scriptName, kInfoColor);
            } else {
                pushConsoleMessage("Lua script moved: " + scriptName, kInfoColor);
            }
            break;
        }
        case Assets::AssetChange::Type::Removed:
            pushConsoleMessage("Lua script removed: " + scriptName, kWarningColor);
            break;
    }
}

void EditorLayer::synchronizeSceneScripts() {
    if (!m_sceneService || !m_scriptAssets) {
        return;
    }
    auto& registry = m_sceneService->context().registry;
    auto scriptsView = registry.view<Scene::LuaScriptComponent>();
    for (auto entity : scriptsView) {
        auto& scriptComponent = scriptsView.get<Scene::LuaScriptComponent>(entity);
        if (const auto* asset = findScriptByName(scriptComponent.scriptName)) {
            synchronizeScriptFields(scriptComponent, *asset);
            if (m_luaRuntime) {
                m_luaRuntime->onScriptAssigned(entity, scriptComponent);
            }
        } else if (!scriptComponent.scriptName.empty()) {
            scriptComponent.serializedFields.clear();
        }
    }
}

void EditorLayer::onScriptPropertiesModified(entt::entity entity) {
    if (!m_sceneService || !m_luaRuntime) {
        return;
    }
    auto& registry = m_sceneService->context().registry;
    if (!registry.valid(entity)) {
        return;
    }
    auto* script = registry.try_get<Scene::LuaScriptComponent>(entity);
    if (!script) {
        return;
    }
    m_luaRuntime->onScriptPropertiesChanged(entity, *script);
}

void EditorLayer::synchronizeScriptFields(Scene::LuaScriptComponent& component, const Scripting::ScriptAsset& asset) const {
    std::unordered_map<std::string, Scene::LuaPropertyValue> nextFields;
    nextFields.reserve(asset.properties.size());
    for (const auto& definition : asset.properties) {
        auto existing = component.serializedFields.find(definition.name);
        switch (definition.type) {
            case Scripting::ScriptPropertyType::Boolean: {
                bool value = definition.boolDefault;
                if (existing != component.serializedFields.end() && std::holds_alternative<bool>(existing->second)) {
                    value = std::get<bool>(existing->second);
                }
                nextFields.emplace(definition.name, value);
                break;
            }
            case Scripting::ScriptPropertyType::String: {
                std::string value = definition.stringDefault;
                if (existing != component.serializedFields.end() && std::holds_alternative<std::string>(existing->second)) {
                    value = std::get<std::string>(existing->second);
                }
                nextFields.emplace(definition.name, value);
                break;
            }
            case Scripting::ScriptPropertyType::Number:
            default: {
                double value = definition.numberDefault;
                if (existing != component.serializedFields.end()) {
                    if (std::holds_alternative<double>(existing->second)) {
                        value = std::get<double>(existing->second);
                    } else if (std::holds_alternative<bool>(existing->second)) {
                        value = std::get<bool>(existing->second) ? 1.0 : 0.0;
                    }
                }
                nextFields.emplace(definition.name, value);
                break;
            }
        }
    }

    component.serializedFields = std::move(nextFields);
}

bool EditorLayer::createLuaScript(const std::string& scriptName) {
    if (!m_scriptAssets) {
        return false;
    }
    if (!IsValidScriptName(scriptName)) {
        m_state.scriptCreationError = "Invalid script name.";
        return false;
    }

    std::string error;
    if (!m_scriptAssets->createScript(scriptName, error)) {
        m_state.scriptCreationError = error;
        return false;
    }

    m_state.scriptCreationError.clear();
    pushConsoleMessage("Created Lua script '" + scriptName + "'", kInfoColor);
    return true;
}

}
