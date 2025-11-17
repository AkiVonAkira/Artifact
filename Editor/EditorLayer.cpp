#include "EditorLayer.h"

#include "Artifact/Core/Application.h"
#include "Artifact/Scripting/LuaContext.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <utility>

#include <glm/gtc/type_ptr.hpp>
#include <misc/cpp/imgui_stdlib.h>
#include <spdlog/spdlog.h>

namespace Artifact::Editor {
namespace {
constexpr ImVec4 kInfoColor{0.6f, 0.78f, 1.0f, 1.0f};
constexpr ImVec4 kWarningColor{1.0f, 0.85f, 0.4f, 1.0f};
constexpr ImVec4 kErrorColor{1.0f, 0.4f, 0.4f, 1.0f};
}

EditorLayer::EditorLayer(Artifact::Core::Application& application)
    : m_application(application) {
    m_runtimeConfig = application.runtimeConfig();
    m_luaContext = application.luaContext();

    refreshScriptAssets();
    createDefaultScene();
    registerPanels();

    pushConsoleMessage("Unity-style editor ready. Happy scripting!", kInfoColor);
}

void EditorLayer::registerPanels() {
    m_application.registerEditorPanel([this]() { drawMainMenuBar(); });
    m_application.registerEditorPanel([this]() { drawToolbar(); });
    m_application.registerEditorPanel([this]() { drawHierarchy(); });
    m_application.registerEditorPanel([this]() { drawInspector(); });
    m_application.registerEditorPanel([this]() { drawSceneView(); });
    m_application.registerEditorPanel([this]() { drawConsole(); });
    m_application.registerEditorPanel([this]() { drawAssets(); });
    m_application.registerEditorPanel([this]() {
        if (m_showDemoWindow) {
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        }
    });
}

void EditorLayer::drawMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                createDefaultScene();
                pushConsoleMessage("New scene created", kInfoColor);
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, false)) {
                pushConsoleMessage("Scene saving not implemented yet", kWarningColor);
            }
            if (ImGui::MenuItem("Exit", "Ctrl+Q")) {
                m_application.requestExit();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Assets")) {
            if (ImGui::MenuItem("Refresh Lua Scripts", "F5")) {
                refreshScriptAssets();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemoWindow);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Open Docs")) {
                pushConsoleMessage("Docs live in docs/ - open Architecture.md for an overview.", kInfoColor);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void EditorLayer::drawToolbar() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 6.0f));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking;
    if (ImGui::Begin("##Toolbar", nullptr, flags)) {
        const bool isPlaying = m_playMode == PlayMode::Playing;
        const bool isPaused = m_isPaused && isPlaying;
        ImVec4 playColor = isPlaying ? ImVec4(0.2f, 0.8f, 0.3f, 1.0f) : ImGui::GetStyle().Colors[ImGuiCol_Button];
        ImVec4 pauseColor = isPaused ? ImVec4(0.95f, 0.8f, 0.2f, 1.0f) : ImGui::GetStyle().Colors[ImGuiCol_Button];

        ImGui::PushStyleColor(ImGuiCol_Button, playColor);
        if (ImGui::Button(isPlaying ? "Stop" : "Play", ImVec2(80.0f, 0.0f))) {
            if (isPlaying) {
                stopPlayMode();
            } else {
                enterPlayMode();
            }
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, pauseColor);
        if (ImGui::Button("Pause", ImVec2(80.0f, 0.0f))) {
            if (m_playMode == PlayMode::Playing) {
                m_isPaused = !m_isPaused;
                pushConsoleMessage(m_isPaused ? "Gameplay paused" : "Gameplay resumed", kInfoColor);
            }
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();

        if (ImGui::Button("Step", ImVec2(80.0f, 0.0f))) {
            stepOnce();
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Scene: %s", m_sceneName.c_str());
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorLayer::drawHierarchy() {
    ImGui::Begin("Hierarchy");
    if (ImGui::Button("Create Empty")) {
        auto& entity = addEntity("GameObject");
        m_selectedEntity = entity.id;
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete")) {
        removeSelectedEntity();
    }
    ImGui::Separator();

    for (auto& entity : m_entities) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (m_selectedEntity == entity.id) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(entity.id)), flags, "%s", entity.name.c_str());
        if (ImGui::IsItemClicked()) {
            m_selectedEntity = entity.id;
        }
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Duplicate")) {
                auto duplicate = entity;
                duplicate.id = 0;
                duplicate.name += " Copy";
                auto& newEntity = addEntity(duplicate.name);
                newEntity.transform = duplicate.transform;
                newEntity.scriptName = duplicate.scriptName;
                newEntity.autoReloadScript = duplicate.autoReloadScript;
            }
            if (ImGui::MenuItem("Delete")) {
                m_selectedEntity = entity.id;
                removeSelectedEntity();
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void EditorLayer::drawInspector() {
    ImGui::Begin("Inspector");
    if (auto* entity = getSelectedEntity()) {
        ImGui::Checkbox("Active", &entity->active);
        ImGui::InputText("Name", &entity->name);

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", glm::value_ptr(entity->transform.position), 0.1f);
            ImGui::DragFloat3("Rotation", glm::value_ptr(entity->transform.rotation), 0.5f);
            ImGui::DragFloat3("Scale", glm::value_ptr(entity->transform.scale), 0.05f, 0.0f, 0.0f, "%.2f");
        }

        if (ImGui::CollapsingHeader("Lua Script", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* current = entity->scriptName.empty() ? "None" : entity->scriptName.c_str();
            if (ImGui::BeginCombo("Script", current)) {
                bool selected = entity->scriptName.empty();
                if (ImGui::Selectable("None", selected)) {
                    entity->scriptName.clear();
                }
                for (const auto& asset : m_scriptAssets) {
                    selected = entity->scriptName == asset.name;
                    if (ImGui::Selectable(asset.name.c_str(), selected)) {
                        entity->scriptName = asset.name;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Checkbox("Auto Reload", &entity->autoReloadScript);
            if (!entity->scriptName.empty()) {
                ImGui::SameLine();
                if (ImGui::Button("Run")) {
                    runEntityScripts();
                }
            }
        }
    } else {
        ImGui::TextUnformatted("Select an entity to inspect its components.");
    }
    ImGui::End();
}

void EditorLayer::drawSceneView() {
    ImGui::Begin("Scene");
    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    const ImVec2 canvasEnd = ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(canvasPos, canvasEnd, IM_COL32(26, 26, 30, 255));

    const float gridSpacing = 32.0f;
    for (float x = std::fmod(canvasPos.x, gridSpacing); x < canvasEnd.x; x += gridSpacing) {
        drawList->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasEnd.y), IM_COL32(40, 40, 48, 255));
    }
    for (float y = std::fmod(canvasPos.y, gridSpacing); y < canvasEnd.y; y += gridSpacing) {
        drawList->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasEnd.x, y), IM_COL32(40, 40, 48, 255));
    }

    drawList->AddText(ImVec2(canvasPos.x + 12.0f, canvasPos.y + 12.0f), IM_COL32_WHITE, "Scene View (stub)");
    if (m_playMode == PlayMode::Playing && !m_isPaused) {
        m_sceneClock += ImGui::GetIO().DeltaTime;
    }
    std::stringstream overlay;
    overlay << (m_playMode == PlayMode::Playing ? "Playing" : "Edit") << (m_isPaused ? " (Paused)" : "");
    overlay << "  |  t=" << std::fixed << std::setprecision(2) << m_sceneClock << "s";
    drawList->AddText(ImVec2(canvasPos.x + 12.0f, canvasPos.y + 32.0f), IM_COL32(200, 200, 200, 255), overlay.str().c_str());

    ImGui::InvisibleButton("scene_canvas", canvasSize);
    ImGui::End();
}

void EditorLayer::drawConsole() {
    ImGui::Begin("Console");
    if (ImGui::Button("Clear")) {
        m_consoleMessages.clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto Scroll", &m_consoleAutoScroll);
    ImGui::Separator();
    m_consoleFilter.Draw("Filter");

    ImGui::BeginChild("ConsoleScroll", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& message : m_consoleMessages) {
        if (m_consoleFilter.PassFilter(message.text.c_str())) {
            ImGui::TextColored(message.color, "%s", message.text.c_str());
        }
    }
    if (m_consoleAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    if (ImGui::InputText("##luaInput", &m_luaInputBuffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
        executeLuaCommand(m_luaInputBuffer);
        m_luaInputBuffer.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Run Lua")) {
        executeLuaCommand(m_luaInputBuffer);
        m_luaInputBuffer.clear();
    }
    ImGui::End();
}

void EditorLayer::drawAssets() {
    ImGui::Begin("Assets");
    if (ImGui::TreeNodeEx("Lua Scripts", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth)) {
        for (const auto& asset : m_scriptAssets) {
            bool selected = false;
            if (auto* entity = getSelectedEntity()) {
                selected = entity->scriptName == asset.name;
            }
            ImGui::Selectable(asset.name.c_str(), selected);
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                assignScriptToSelection(asset.name);
            }
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Assign to selection")) {
                    assignScriptToSelection(asset.name);
                }
                ImGui::EndPopup();
            }
        }
        ImGui::TreePop();
    }
    ImGui::End();
}

void EditorLayer::refreshScriptAssets() {
    ensureScriptsFolderExists();
    m_scriptAssets.clear();
    const auto scriptsRoot = m_runtimeConfig.paths.rawAssets / "Scripts";
    for (const auto& entry : std::filesystem::recursive_directory_iterator(scriptsRoot)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".lua") {
            LuaScriptAsset asset;
            asset.name = entry.path().stem().string();
            asset.path = std::filesystem::absolute(entry.path());
            m_scriptAssets.emplace_back(std::move(asset));
        }
    }
    std::sort(m_scriptAssets.begin(), m_scriptAssets.end(), [](const LuaScriptAsset& a, const LuaScriptAsset& b) {
        return a.name < b.name;
    });

    pushConsoleMessage("Discovered " + std::to_string(m_scriptAssets.size()) + " Lua scripts", kInfoColor);
}

void EditorLayer::ensureScriptsFolderExists() const {
    const auto scriptsRoot = m_runtimeConfig.paths.rawAssets / "Scripts";
    std::error_code ec;
    std::filesystem::create_directories(scriptsRoot, ec);
}

void EditorLayer::createDefaultScene() {
    m_entities.clear();
    m_nextEntityId = 1;
    auto& camera = addEntity("Main Camera");
    camera.transform.position = {0.0f, 5.0f, -10.0f};
    camera.transform.rotation = {20.0f, 0.0f, 0.0f};

    auto& actor = addEntity("LuaActor");
    actor.transform.position = {0.0f, 0.0f, 0.0f};
    if (!m_scriptAssets.empty()) {
        actor.scriptName = m_scriptAssets.front().name;
    }

    m_selectedEntity = actor.id;
}

EditorEntity& EditorLayer::addEntity(const std::string& name) {
    EditorEntity entity;
    entity.id = m_nextEntityId++;
    entity.name = name;
    entity.transform.scale = {1.0f, 1.0f, 1.0f};
    m_entities.emplace_back(std::move(entity));
    return m_entities.back();
}

void EditorLayer::removeSelectedEntity() {
    if (m_selectedEntity < 0) {
        return;
    }
    const int idToRemove = m_selectedEntity;
    m_entities.erase(std::remove_if(m_entities.begin(), m_entities.end(), [idToRemove](const EditorEntity& entity) {
                                return entity.id == idToRemove;
                            }),
        m_entities.end());
    m_selectedEntity = m_entities.empty() ? -1 : m_entities.front().id;
}

EditorEntity* EditorLayer::getSelectedEntity() {
    if (m_selectedEntity < 0) {
        return nullptr;
    }
    auto it = std::find_if(m_entities.begin(), m_entities.end(), [this](const EditorEntity& entity) {
        return entity.id == m_selectedEntity;
    });
    if (it == m_entities.end()) {
        return nullptr;
    }
    return &(*it);
}

const LuaScriptAsset* EditorLayer::findScriptByName(const std::string& name) const {
    auto it = std::find_if(m_scriptAssets.begin(), m_scriptAssets.end(), [&name](const LuaScriptAsset& asset) {
        return asset.name == name;
    });
    if (it == m_scriptAssets.end()) {
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
    m_consoleMessages.push_back(ConsoleMessage{message, color});
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
    if (m_playMode == PlayMode::Playing) {
        return;
    }
    m_playMode = PlayMode::Playing;
    m_sceneClock = 0.0f;
    m_isPaused = false;
    pushConsoleMessage("Entering play mode", kInfoColor);
    runEntityScripts();
}

void EditorLayer::stopPlayMode() {
    if (m_playMode == PlayMode::Edit) {
        return;
    }
    m_playMode = PlayMode::Edit;
    m_sceneClock = 0.0f;
    m_isPaused = false;
    pushConsoleMessage("Exited play mode", kInfoColor);
}

void EditorLayer::stepOnce() {
    if (m_playMode != PlayMode::Playing) {
        enterPlayMode();
    }
    m_isPaused = true;
    pushConsoleMessage("Stepped one frame", kInfoColor);
    runEntityScripts();
}

void EditorLayer::runEntityScripts() {
    if (!m_luaContext) {
        pushConsoleMessage("No Lua context available", kErrorColor);
        return;
    }

    for (const auto& entity : m_entities) {
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

}
