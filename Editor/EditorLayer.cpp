#include "EditorLayer.h"

#include "Artifact/Core/Application.h"
#include "Artifact/Rendering/Renderer.h"
#include "Artifact/Scripting/LuaContext.h"

#include <GLFW/glfw3.h>

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
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>

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
}

EditorLayer::EditorLayer(Artifact::Core::Application& application)
    : m_application(application) {
    m_runtimeConfig = application.runtimeConfig();
    m_luaContext = application.luaContext();
    m_renderer = application.renderer();

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
            PanelContext ctx{m_state, *this};
            panelPtr->draw(ctx);
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

    m_renderer->beginSceneViewport(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
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
    m_state.entities.clear();
    m_state.nextEntityId = 1;
    auto& camera = addEntity("Main Camera");
    camera.transform.position = {0.0f, 5.0f, -10.0f};
    camera.transform.rotation = {20.0f, 0.0f, 0.0f};

    auto& actor = addEntity("LuaActor");
    actor.transform.position = {0.0f, 0.0f, 0.0f};
    if (!m_state.scriptAssets.empty()) {
        actor.scriptName = m_state.scriptAssets.front().name;
    }

    m_state.selectedEntity = actor.id;
}

EditorEntity& EditorLayer::addEntity(const std::string& name) {
    EditorEntity entity;
    entity.id = m_state.nextEntityId++;
    entity.name = name;
    entity.transform.scale = {1.0f, 1.0f, 1.0f};
    m_state.entities.emplace_back(std::move(entity));
    return m_state.entities.back();
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

void EditorLayer::removeSelectedEntity() {
    if (m_state.selectedEntity < 0) {
        return;
    }
    const int idToRemove = m_state.selectedEntity;
    m_state.entities.erase(std::remove_if(m_state.entities.begin(), m_state.entities.end(), [idToRemove](const EditorEntity& entity) {
                                return entity.id == idToRemove;
                            }),
        m_state.entities.end());
    m_state.selectedEntity = m_state.entities.empty() ? -1 : m_state.entities.front().id;
}

EditorEntity* EditorLayer::getSelectedEntity() {
    if (m_state.selectedEntity < 0) {
        return nullptr;
    }
    auto it = std::find_if(m_state.entities.begin(), m_state.entities.end(), [this](const EditorEntity& entity) {
        return entity.id == m_state.selectedEntity;
    });
    if (it == m_state.entities.end()) {
        return nullptr;
    }
    return &(*it);
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
    if (auto* entity = getSelectedEntity()) {
        entity->scriptName = scriptName;
        pushConsoleMessage("Assigned script '" + scriptName + "' to " + entity->name, kInfoColor);
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
    if (!m_luaContext) {
        pushConsoleMessage("No Lua context available", kErrorColor);
        return;
    }

    (void)deltaTime;

    for (const auto& entity : m_state.entities) {
        if (entity.scriptName.empty()) {
            continue;
        }
        if (const auto* asset = findScriptByName(entity.scriptName)) {
            m_luaContext->executeFile(asset->path);
            pushConsoleMessage("Executed script '" + entity.scriptName + "'", kInfoColor);
        } else {
            pushConsoleMessage("Missing script '" + entity.scriptName + "'", kWarningColor);
        }
    }
}

void EditorLayer::requestExit() {
    m_application.requestExit();
}

void EditorLayer::resetLayout() {
    m_application.resetLayout();
}

}
