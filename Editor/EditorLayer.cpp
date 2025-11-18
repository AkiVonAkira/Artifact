#include "EditorLayer.h"

#include "Artifact/Core/Application.h"
#include "Artifact/Rendering/Renderer.h"
#include "Artifact/Scripting/LuaContext.h"
#include "Artifact/Scene/Prefabs.h"
#include "Artifact/Scene/SceneContext.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <utility>
#include <cctype>

#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>
#include <entt/entt.hpp>

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

} // namespace

EditorLayer::EditorLayer(Artifact::Core::Application& application)
    : m_application(application) {
    m_runtimeConfig = application.runtimeConfig();
    m_luaContext = application.luaContext();
    m_renderer = application.renderer();
    if (m_luaContext) {
        m_luaBindings = std::make_unique<Scripting::LuaBindings>(*m_luaContext);
        m_luaBindings->registerInputBindings([this](int key) {
            return m_application.isKeyDown(key);
        });
        m_luaBindings->registerTimeBindings();
    m_luaBindings->registerEntityBindings();
    m_luaBindings->setRegistry(&m_state.scene.registry);
    }

    refreshScriptAssets();
    createDefaultScene();

    registerPanels();
    m_application.setStatusBarCallback([this]() {
        drawStatusBarContent();
    });
    pushConsoleMessage("Unity-style editor ready. Happy scripting!", kInfoColor);
}

void EditorLayer::registerPanels() {
    PanelContext ctx{m_state, *this};
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
            PanelContext panelCtx{m_state, *this};
            panelPtr->draw(panelCtx);
        });
    }
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

    const bool isPlaying = m_state.playMode == PlayMode::Playing;
    const bool isPaused = m_state.isPaused && isPlaying;
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
        if (m_state.playMode == PlayMode::Playing) {
            m_state.isPaused = !m_state.isPaused;
            pushConsoleMessage(m_state.isPaused ? "Gameplay paused" : "Gameplay resumed", kInfoColor);
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

    if (m_state.playMode == PlayMode::Playing && !m_state.isPaused) {
        const float deltaTime = ImGui::GetIO().DeltaTime;
        m_state.sceneClock += deltaTime;
        runEntityScripts(deltaTime);
    }

    m_renderer->beginSceneViewport(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
    m_renderer->drawSkybox();
    // TODO: render actual scene content once ECS/renderer hookup exists.
    m_renderer->endSceneViewport();

    const uint32_t texture = m_renderer->sceneColorTexture();
    if (texture != 0) {
        ImGui::Image(texture, viewportSize, ImVec2(0, 1), ImVec2(1, 0));
    }
}

void EditorLayer::refreshScriptAssets() {
    ensureScriptsFolderExists();
    m_state.scriptAssets.clear();
    const auto scriptsRoot = m_runtimeConfig.paths.rawAssets / "Scripts";
    for (const auto& entry : std::filesystem::recursive_directory_iterator(scriptsRoot)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".lua") {
            LuaScriptAsset asset;
            asset.name = entry.path().stem().string();
            asset.path = std::filesystem::absolute(entry.path());
            m_state.scriptAssets.emplace_back(std::move(asset));
        }
    }
    std::sort(m_state.scriptAssets.begin(), m_state.scriptAssets.end(), [](const LuaScriptAsset& a, const LuaScriptAsset& b) {
        return a.name < b.name;
    });

    pushConsoleMessage("Discovered " + std::to_string(m_state.scriptAssets.size()) + " Lua scripts", kInfoColor);
}

void EditorLayer::ensureScriptsFolderExists() const {
    const auto scriptsRoot = m_runtimeConfig.paths.rawAssets / "Scripts";
    std::error_code ec;
    std::filesystem::create_directories(scriptsRoot, ec);
}

void EditorLayer::createDefaultScene() {
    Scene::ResetScene(m_state.scene);
    m_state.selectedEntity = entt::null;

    Scene::Prefabs::CameraPrefabOptions cameraOptions;
    if (findScriptByName("CameraController")) {
        cameraOptions.scriptName = "CameraController";
    }
    Scene::Prefabs::CreateMainCamera(m_state.scene, cameraOptions);

    Scene::Prefabs::LuaActorPrefabOptions actorOptions;
    if (!m_state.scriptAssets.empty()) {
        actorOptions.scriptName = m_state.scriptAssets.front().name;
    }
    const auto actor = Scene::Prefabs::CreateLuaActor(m_state.scene, actorOptions);

    selectEntity(actor);
}

entt::entity EditorLayer::addEntity(const std::string& name) {
    return Scene::CreateEntity(m_state.scene, name);
}

bool EditorLayer::createLuaScript(const std::string& scriptName) {
    if (!IsValidScriptName(scriptName)) {
        m_state.scriptCreationError = "Invalid script name.";
        return false;
    }

    ensureScriptsFolderExists();
    const auto scriptsRoot = m_runtimeConfig.paths.rawAssets / "Scripts";
    const auto newScriptPath = scriptsRoot / (scriptName + ".lua");
    if (std::filesystem::exists(newScriptPath)) {
        m_state.scriptCreationError = "Script already exists.";
        return false;
    }

    std::ofstream file(newScriptPath);
    if (!file.is_open()) {
        m_state.scriptCreationError = "Failed to create script on disk.";
        return false;
    }

    file << "-- " << scriptName << "\n\n"
         << "function update(dt)\n"
         << "    -- Called every frame while in Play mode\n"
         << "end\n";
    file.close();

    refreshScriptAssets();
    pushConsoleMessage("Created Lua script '" + scriptName + "'", kInfoColor);
    return true;
}

void EditorLayer::drawStatusBarContent() {
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Scene: %s", m_state.sceneName.c_str());
}

std::string EditorLayer::trimString(const std::string& value) const {
    return TrimString(value);
}

void EditorLayer::duplicateEntity(entt::entity entity) {
    auto& registry = m_state.scene.registry;
    if (!registry.valid(entity)) {
        return;
    }
    const auto& nameComponent = registry.get<NameComponent>(entity);
    entt::entity duplicate = addEntity(nameComponent.name + " Copy");

    if (auto* sourceTransform = registry.try_get<TransformComponent>(entity)) {
        auto& destTransform = registry.get<TransformComponent>(duplicate);
        destTransform.transform = sourceTransform->transform;
    }
    if (auto* sourceScript = registry.try_get<LuaScriptComponent>(entity)) {
        auto& destScript = registry.get<LuaScriptComponent>(duplicate);
        destScript.scriptName = sourceScript->scriptName;
        destScript.autoReloadScript = sourceScript->autoReloadScript;
    }
    if (auto* sourceActive = registry.try_get<ActiveComponent>(entity)) {
        auto& destActive = registry.get<ActiveComponent>(duplicate);
        destActive.active = sourceActive->active;
    }

    selectEntity(duplicate);
}

void EditorLayer::removeSelectedEntity() {
    auto& registry = m_state.scene.registry;
    if (m_state.selectedEntity != entt::null && registry.valid(m_state.selectedEntity)) {
        registry.destroy(m_state.selectedEntity);
        m_state.selectedEntity = entt::null;

        auto view = registry.view<IdComponent>();
        auto it = view.begin();
        if (it != view.end()) {
            m_state.selectedEntity = *it;
        }
    }
}

void EditorLayer::selectEntity(entt::entity entity) {
    auto& registry = m_state.scene.registry;
    if (entity != entt::null && !registry.valid(entity)) {
        m_state.selectedEntity = entt::null;
        return;
    }
    m_state.selectedEntity = entity;
}

entt::entity EditorLayer::getSelectedEntity() const {
    const auto& registry = m_state.scene.registry;
    if (m_state.selectedEntity != entt::null && registry.valid(m_state.selectedEntity)) {
        return m_state.selectedEntity;
    }
    return entt::null;
}

const LuaScriptAsset* EditorLayer::findScriptByName(const std::string& name) const {
    auto it = std::find_if(m_state.scriptAssets.begin(), m_state.scriptAssets.end(), [&name](const LuaScriptAsset& asset) {
        return asset.name == name;
    });
    if (it == m_state.scriptAssets.end()) {
        return nullptr;
    }
    return &(*it);
}

void EditorLayer::assignScriptToSelection(const std::string& scriptName) {
    const auto selected = getSelectedEntity();
    if (selected == entt::null) {
        return;
    }
    auto& registry = m_state.scene.registry;
    if (auto* script = registry.try_get<LuaScriptComponent>(selected)) {
        script->scriptName = scriptName;
        const auto* nameComponent = registry.try_get<NameComponent>(selected);
        const std::string entityName = nameComponent ? nameComponent->name : "Entity";
        pushConsoleMessage("Assigned script '" + scriptName + "' to " + entityName, kInfoColor);
    }
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
    if (m_state.playMode == PlayMode::Playing) {
        return;
    }
    m_state.playMode = PlayMode::Playing;
    m_state.sceneClock = 0.0f;
    m_state.isPaused = false;
    pushConsoleMessage("Entering play mode", kInfoColor);
    runEntityScripts(0.0f);
}

void EditorLayer::stopPlayMode() {
    if (m_state.playMode == PlayMode::Edit) {
        return;
    }
    m_state.playMode = PlayMode::Edit;
    m_state.sceneClock = 0.0f;
    m_state.isPaused = false;
    pushConsoleMessage("Exited play mode", kInfoColor);
}

void EditorLayer::stepOnce() {
    if (m_state.playMode != PlayMode::Playing) {
        enterPlayMode();
    }
    m_state.isPaused = true;
    pushConsoleMessage("Stepped one frame", kInfoColor);
    runEntityScripts(1.0f / 60.0f);
}

void EditorLayer::runEntityScripts(float deltaTime) {
    if (!m_luaContext || !m_luaBindings) {
        pushConsoleMessage("No Lua context available", kErrorColor);
        return;
    }

    m_luaBindings->updateDeltaTime(deltaTime);

    auto& registry = m_state.scene.registry;
    auto view = registry.view<NameComponent, LuaScriptComponent>();
    for (auto entity : view) {
        auto [nameComponent, scriptComponent] = view.get<NameComponent, LuaScriptComponent>(entity);
        if (scriptComponent.scriptName.empty()) {
            continue;
        }

        if (const auto* asset = findScriptByName(scriptComponent.scriptName)) {
            m_luaBindings->setActiveEntity(entity);
            int entityId = -1;
            if (auto* idComp = registry.try_get<IdComponent>(entity)) {
                entityId = idComp->id;
            }
            m_luaBindings->updateEntityMetadata(nameComponent.name, entityId);
            m_luaContext->executeFile(asset->path);
        } else {
            pushConsoleMessage("Missing script '" + scriptComponent.scriptName + "' for " + nameComponent.name, kWarningColor);
        }
        m_luaBindings->clearActiveEntity();
        m_luaBindings->clearEntityMetadata();
    }
}

void EditorLayer::requestExit() {
    m_application.requestExit();
}

void EditorLayer::resetLayout() {
    m_application.resetLayout();
}

}