#include "EditorLayer.h"

#include "Artifact/Assets/AssetWatcher.h"
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
#include <optional>
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
#include <sol/sol.hpp>

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

        LuaPropertyType ParsePropertyType(const std::string& typeString) {
            std::string lowered;
            lowered.resize(typeString.size());
            std::transform(typeString.begin(), typeString.end(), lowered.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            if (lowered == "bool" || lowered == "boolean") {
                return LuaPropertyType::Boolean;
            }
            if (lowered == "string" || lowered == "text") {
                return LuaPropertyType::String;
            }
            return LuaPropertyType::Number;
        }

        LuaPropertyValue MakeDefaultValue(const LuaScriptPropertyDefinition& definition) {
            switch (definition.type) {
                case LuaPropertyType::Boolean:
                    return definition.boolDefault;
                case LuaPropertyType::String:
                    return definition.stringDefault;
                case LuaPropertyType::Number:
                default:
                    return definition.numberDefault;
            }
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

        if (m_state.playMode == PlayMode::Playing) {
            const float deltaTime = ImGui::GetIO().DeltaTime;
            if (!m_state.isPaused) {
                m_state.sceneClock += deltaTime;
            }
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

    void EditorLayer::refreshScriptAssets(bool announce) {
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
                asset.properties = parseScriptProperties(asset.path);
                m_state.scriptAssets.emplace_back(std::move(asset));
            }
        }
        std::sort(m_state.scriptAssets.begin(), m_state.scriptAssets.end(), [](const LuaScriptAsset& a, const LuaScriptAsset& b) {
            return a.name < b.name;
        });

        auto& registry = m_state.scene.registry;
        auto scriptsView = registry.view<LuaScriptComponent>();
        for (auto entity : scriptsView) {
            auto& scriptComponent = scriptsView.get<LuaScriptComponent>(entity);
            if (const auto* asset = findScriptByName(scriptComponent.scriptName)) {
                synchronizeScriptFields(scriptComponent, *asset);
            } else if (!scriptComponent.scriptName.empty()) {
                scriptComponent.serializedFields.clear();
            }
        }

        m_scriptRuntimes.clear();

        if (announce) {
            pushConsoleMessage("Discovered " + std::to_string(m_state.scriptAssets.size()) + " Lua scripts", kInfoColor);
        }
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
        const auto camera = Scene::Prefabs::CreateMainCamera(m_state.scene, cameraOptions);
        initializeScriptComponent(camera);

        Scene::Prefabs::LuaActorPrefabOptions actorOptions;
        if (!m_state.scriptAssets.empty()) {
            actorOptions.scriptName = m_state.scriptAssets.front().name;
        }
        const auto actor = Scene::Prefabs::CreateLuaActor(m_state.scene, actorOptions);
        initializeScriptComponent(actor);

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

        file << "local " << scriptName << " = {\n"
            << "    properties = {\n"
            << "        exampleValue = { type = \"float\", default = 1.0 }\n"
            << "    }\n"
            << "}\n\n"
            << "function " << scriptName << ":awake()\n"
            << "    -- Called once when the entity enters Play mode\n"
            << "end\n\n"
            << "function " << scriptName << ":start()\n"
            << "    -- Called on the first frame after awake\n"
            << "end\n\n"
            << "function " << scriptName << ":update(dt)\n"
            << "    -- Called every frame while the entity is playing\n"
            << "end\n\n"
            << "return " << scriptName << "\n";
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
            destScript.enabled = sourceScript->enabled;
            destScript.autoReloadScript = sourceScript->autoReloadScript;
            destScript.serializedFields = sourceScript->serializedFields;
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
            const bool changedScript = script->scriptName != scriptName;
            script->scriptName = scriptName;
            if (changedScript) {
                destroyRuntime(selected);
                if (const auto* asset = findScriptByName(scriptName)) {
                    synchronizeScriptFields(*script, *asset);
                } else {
                    script->serializedFields.clear();
                }
            }
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
        m_scriptRuntimes.clear();
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
        m_scriptRuntimes.clear();
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
        for (auto it = m_scriptRuntimes.begin(); it != m_scriptRuntimes.end();) {
            if (!registry.valid(it->first) || !registry.all_of<LuaScriptComponent>(it->first)) {
                it = m_scriptRuntimes.erase(it);
            } else {
                ++it;
            }
        }

        auto view = registry.view<NameComponent, LuaScriptComponent, ActiveComponent>();
        for (auto entity : view) {
            auto [nameComponent, scriptComponent, activeComponent] = view.get<NameComponent, LuaScriptComponent, ActiveComponent>(entity);
            if (scriptComponent.scriptName.empty() || !activeComponent.active) {
                destroyRuntime(entity);
                continue;
            }

            auto* runtime = getOrCreateRuntime(entity, scriptComponent);
            if (!runtime) {
                continue;
            }

            m_luaBindings->setActiveEntity(entity);
            int entityId = -1;
            if (auto* idComp = registry.try_get<IdComponent>(entity)) {
                entityId = idComp->id;
            }
            m_luaBindings->updateEntityMetadata(nameComponent.name, entityId);

            if (!runtime->awakeCalled) {
                InvokeLifecycle(runtime->awake, runtime->instance, "awake");
                runtime->awakeCalled = true;
            }

            if (scriptComponent.enabled && !runtime->enabledLastFrame) {
                InvokeLifecycle(runtime->onEnable, runtime->instance, "onEnable");
            } else if (!scriptComponent.enabled && runtime->enabledLastFrame) {
                InvokeLifecycle(runtime->onDisable, runtime->instance, "onDisable");
            }

            runtime->enabledLastFrame = scriptComponent.enabled;

            if (scriptComponent.enabled) {
                if (!runtime->startCalled) {
                    InvokeLifecycle(runtime->start, runtime->instance, "start");
                    runtime->startCalled = true;
                }

                if (m_state.playMode == PlayMode::Playing && !m_state.isPaused) {
                    InvokeLifecycle(runtime->update, runtime->instance, "update", deltaTime);
                }
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

        m_scriptRuntimes.clear();
    }

    std::vector<LuaScriptPropertyDefinition> EditorLayer::parseScriptProperties(const std::filesystem::path& path) const {
        std::vector<LuaScriptPropertyDefinition> properties;
        if (!m_luaContext) {
            return properties;
        }

        auto& lua = m_luaContext->state();
        sol::environment env(lua, sol::create, lua.globals());
        sol::protected_function_result execResult = lua.safe_script_file(path.string(), env, sol::script_pass_on_error);
        if (!execResult.valid()) {
            sol::error err = execResult;
            spdlog::warn("[Editor] Failed to parse Lua script '{}': {}", path.string(), err.what());
            return properties;
        }

        sol::object definitionObject;
        if (execResult.return_count() > 0) {
            definitionObject = execResult.get<sol::object>();
        }

        sol::table definitionTable;
        if (definitionObject.valid() && definitionObject.get_type() == sol::type::table) {
            definitionTable = definitionObject;
        } else {
            definitionTable = env;
        }

        sol::object propertiesObject = definitionTable["properties"];
        if (!propertiesObject.valid() || propertiesObject.get_type() != sol::type::table) {
            return properties;
        }

        sol::table propertiesTable = propertiesObject;
        for (auto&& pair : propertiesTable) {
            sol::object key = pair.first;
            sol::object value = pair.second;
            if (key.get_type() != sol::type::string || value.get_type() != sol::type::table) {
                continue;
            }

            LuaScriptPropertyDefinition definition;
            definition.name = key.as<std::string>();

            sol::table propertyTable = value;
            const std::string typeString = propertyTable.get_or("type", std::string{"float"});
            definition.type = ParsePropertyType(typeString);

            switch (definition.type) {
                case LuaPropertyType::Boolean: {
                    sol::object defaultObject = propertyTable.raw_get<sol::object>("default");
                    if (defaultObject.valid() && defaultObject.get_type() == sol::type::boolean) {
                        definition.boolDefault = defaultObject.as<bool>();
                    } else {
                        definition.boolDefault = true;
                    }
                    break;
                }
                case LuaPropertyType::String: {
                    sol::object defaultObject = propertyTable.raw_get<sol::object>("default");
                    if (defaultObject.valid() && defaultObject.get_type() == sol::type::string) {
                        definition.stringDefault = defaultObject.as<std::string>();
                    } else {
                        definition.stringDefault.clear();
                    }
                    break;
                }
                case LuaPropertyType::Number:
                default: {
                    sol::object defaultObject = propertyTable.raw_get<sol::object>("default");
                    if (defaultObject.valid() && defaultObject.get_type() == sol::type::number) {
                        definition.numberDefault = defaultObject.as<double>();
                    } else {
                        definition.numberDefault = 0.0;
                    }

                    sol::object minObject = propertyTable.raw_get<sol::object>("min");
                    if (minObject.valid() && minObject.get_type() == sol::type::number) {
                        definition.minValue = minObject.as<double>();
                    }

                    sol::object maxObject = propertyTable.raw_get<sol::object>("max");
                    if (maxObject.valid() && maxObject.get_type() == sol::type::number) {
                        definition.maxValue = maxObject.as<double>();
                    }
                    break;
                }
            }

            properties.emplace_back(std::move(definition));
        }

        return properties;
    }

    void EditorLayer::synchronizeScriptFields(LuaScriptComponent& component, const LuaScriptAsset& asset) const {
        std::unordered_map<std::string, LuaPropertyValue> nextFields;
        nextFields.reserve(asset.properties.size());
        for (const auto& definition : asset.properties) {
            auto existing = component.serializedFields.find(definition.name);
            switch (definition.type) {
                case LuaPropertyType::Boolean: {
                    bool value = definition.boolDefault;
                    if (existing != component.serializedFields.end() && std::holds_alternative<bool>(existing->second)) {
                        value = std::get<bool>(existing->second);
                    }
                    nextFields.emplace(definition.name, value);
                    break;
                }
                case LuaPropertyType::String: {
                    std::string value = definition.stringDefault;
                    if (existing != component.serializedFields.end() && std::holds_alternative<std::string>(existing->second)) {
                        value = std::get<std::string>(existing->second);
                    }
                    nextFields.emplace(definition.name, value);
                    break;
                }
                case LuaPropertyType::Number:
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

    void EditorLayer::initializeScriptComponent(entt::entity entity) {
        auto& registry = m_state.scene.registry;
        if (!registry.valid(entity)) {
            return;
        }

        auto* script = registry.try_get<LuaScriptComponent>(entity);
        if (!script) {
            return;
        }

        if (script->scriptName.empty()) {
            script->serializedFields.clear();
            return;
        }

        if (const auto* asset = findScriptByName(script->scriptName)) {
            synchronizeScriptFields(*script, *asset);
        } else {
            script->serializedFields.clear();
        }
    }

    EditorLayer::ScriptRuntimeState* EditorLayer::findRuntime(entt::entity entity) {
        auto it = m_scriptRuntimes.find(entity);
        if (it == m_scriptRuntimes.end()) {
            return nullptr;
        }
        return &it->second;
    }

    void EditorLayer::destroyRuntime(entt::entity entity) {
        m_scriptRuntimes.erase(entity);
    }

    EditorLayer::ScriptRuntimeState* EditorLayer::getOrCreateRuntime(entt::entity entity, LuaScriptComponent& component) {
        if (component.scriptName.empty()) {
            destroyRuntime(entity);
            return nullptr;
        }

        const auto* asset = findScriptByName(component.scriptName);
        if (!asset) {
            pushConsoleMessage("Missing script '" + component.scriptName + "'", kWarningColor);
            destroyRuntime(entity);
            return nullptr;
        }

        if (auto* existing = findRuntime(entity)) {
            if (existing->scriptName == component.scriptName) {
                applyPropertiesToRuntime(*existing, component, *asset);
                return existing;
            }
            destroyRuntime(entity);
        }

        if (!m_luaContext) {
            return nullptr;
        }

        auto& lua = m_luaContext->state();
        ScriptRuntimeState runtime{};
        runtime.scriptName = component.scriptName;
        runtime.environment = sol::environment(lua, sol::create, lua.globals());
        sol::protected_function_result execResult = lua.safe_script_file(asset->path.string(), runtime.environment, sol::script_pass_on_error);
        if (!execResult.valid()) {
            sol::error err = execResult;
            spdlog::error("[Lua] Failed to instantiate '{}': {}", asset->name, err.what());
            return nullptr;
        }

        sol::object definitionObject;
        if (execResult.return_count() > 0) {
            definitionObject = execResult.get<sol::object>();
        }

        if (definitionObject.valid() && definitionObject.get_type() == sol::type::table) {
            runtime.definition = definitionObject;
        } else {
            runtime.definition = runtime.environment;
        }

        runtime.instance = lua.create_table();
        runtime.properties = lua.create_table();
        runtime.instance["properties"] = runtime.properties;

        sol::table metatable = lua.create_table();
        metatable["__index"] = runtime.definition;
        runtime.instance[sol::metatable_key] = metatable;

        runtime.awake = runtime.definition["awake"];
        runtime.start = runtime.definition["start"];
        runtime.update = runtime.definition["update"];
        runtime.onEnable = runtime.definition["on_enable"];
        if (!runtime.onEnable.valid()) {
            runtime.onEnable = runtime.definition["onEnable"];
        }
        runtime.onDisable = runtime.definition["on_disable"];
        if (!runtime.onDisable.valid()) {
            runtime.onDisable = runtime.definition["onDisable"];
        }

        auto [it, inserted] = m_scriptRuntimes.emplace(entity, std::move(runtime));
        ScriptRuntimeState& stored = it->second;
        stored.awakeCalled = false;
        stored.startCalled = false;
        stored.enabledLastFrame = false;
        applyPropertiesToRuntime(stored, component, *asset);
        return &stored;
    }

    void EditorLayer::applyPropertiesToRuntime(ScriptRuntimeState& runtime,
                                               const LuaScriptComponent& component,
                                               const LuaScriptAsset& asset) {
        if (!runtime.properties.valid()) {
            return;
        }

        runtime.properties.clear();
        for (const auto& definition : asset.properties) {
            LuaPropertyValue value = MakeDefaultValue(definition);
            auto existing = component.serializedFields.find(definition.name);
            if (existing != component.serializedFields.end()) {
                switch (definition.type) {
                    case LuaPropertyType::Boolean:
                        if (std::holds_alternative<bool>(existing->second)) {
                            value = existing->second;
                        }
                        break;
                    case LuaPropertyType::String:
                        if (std::holds_alternative<std::string>(existing->second)) {
                            value = existing->second;
                        }
                        break;
                    case LuaPropertyType::Number:
                    default:
                        if (std::holds_alternative<double>(existing->second)) {
                            value = existing->second;
                        }
                        break;
                }
            }

            std::visit([&](auto&& arg) {
                runtime.properties[definition.name] = arg;
            }, value);
        }
    }

    void EditorLayer::onScriptPropertiesModified(entt::entity entity) {
        auto& registry = m_state.scene.registry;
        if (!registry.valid(entity)) {
            return;
        }
        auto* script = registry.try_get<LuaScriptComponent>(entity);
        if (!script) {
            return;
        }
        auto* runtime = findRuntime(entity);
        if (!runtime) {
            return;
        }
        if (const auto* asset = findScriptByName(script->scriptName)) {
            applyPropertiesToRuntime(*runtime, *script, *asset);
        }
    }
}