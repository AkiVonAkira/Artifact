#include "Artifact/Scripting/LuaRuntimeSystem.h"

#include <spdlog/spdlog.h>

#include "Artifact/Scripting/LuaBindings.h"
#include "Artifact/Scripting/LuaContext.h"

namespace Artifact::Scripting {
    namespace {
        void InvokeLifecycle(const sol::protected_function &fn,
                             const sol::table &instance,
                             const char *label,
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

    LuaRuntimeSystem::LuaRuntimeSystem(LuaContext &context, ScriptAssetService &assetService, LuaBindings &bindings)
        : m_context(context)
          , m_assets(assetService)
          , m_bindings(bindings) {
    }

    void LuaRuntimeSystem::tick(entt::registry &registry, float deltaTime, bool shouldUpdate) {
        if (!shouldUpdate) {
            return;
        }

        m_bindings.updateDeltaTime(deltaTime);

        for (auto it = m_runtimes.begin(); it != m_runtimes.end();) {
            if (!registry.valid(it->first) || !registry.all_of<Scene::LuaScriptComponent>(it->first)) {
                it = m_runtimes.erase(it);
            } else {
                ++it;
            }
        }

        auto view = registry.view<Scene::NameComponent, Scene::LuaScriptComponent, Scene::ActiveComponent>();
        for (auto entity: view) {
            auto [nameComponent, scriptComponent, activeComponent] = view.get<Scene::NameComponent,
                Scene::LuaScriptComponent, Scene::ActiveComponent>(entity);
            if (scriptComponent.scriptName.empty() || !activeComponent.active) {
                destroyRuntime(entity);
                continue;
            }

            auto *runtime = getOrCreateRuntime(entity, scriptComponent);
            if (!runtime) {
                continue;
            }

            m_bindings.setActiveEntity(entity);
            int entityId = -1;
            if (auto *idComp = registry.try_get<Scene::IdComponent>(entity)) {
                entityId = idComp->id;
            }
            m_bindings.updateEntityMetadata(nameComponent.name, entityId);

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

                if (!m_assets.scripts().empty()) {
                    InvokeLifecycle(runtime->update, runtime->instance, "update", deltaTime);
                }
            }

            m_bindings.clearActiveEntity();
            m_bindings.clearEntityMetadata();
        }
    }

    void LuaRuntimeSystem::clear() {
        m_runtimes.clear();
    }

    void LuaRuntimeSystem::onScriptAssigned(entt::entity entity, Scene::LuaScriptComponent &component) {
        destroyRuntime(entity);
        getOrCreateRuntime(entity, component);
    }

    void LuaRuntimeSystem::onScriptPropertiesChanged(entt::entity entity, const Scene::LuaScriptComponent &component) {
        auto *runtime = findRuntime(entity);
        if (!runtime) {
            return;
        }
        if (const auto *asset = m_assets.findByName(component.scriptName)) {
            applyProperties(*runtime, component, *asset);
        }
    }

    ScriptRuntimeState *
    LuaRuntimeSystem::getOrCreateRuntime(entt::entity entity, Scene::LuaScriptComponent &component) {
        if (component.scriptName.empty()) {
            destroyRuntime(entity);
            return nullptr;
        }

        const auto *asset = m_assets.findByName(component.scriptName);
        if (!asset) {
            destroyRuntime(entity);
            return nullptr;
        }

        if (auto *existing = findRuntime(entity)) {
            if (existing->scriptName == component.scriptName) {
                applyProperties(*existing, component, *asset);
                return existing;
            }
            destroyRuntime(entity);
        }

        auto &lua = m_context.state();
        ScriptRuntimeState runtime{};
        runtime.scriptName = component.scriptName;
        runtime.environment = sol::environment(lua, sol::create, lua.globals());
        sol::protected_function_result execResult = lua.safe_script_file(
            asset->path.string(), runtime.environment, sol::script_pass_on_error);
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

        auto [it, inserted] = m_runtimes.emplace(entity, std::move(runtime));
        ScriptRuntimeState &stored = it->second;
        stored.awakeCalled = false;
        stored.startCalled = false;
        stored.enabledLastFrame = false;
        applyProperties(stored, component, *asset);
        return &stored;
    }

    ScriptRuntimeState *LuaRuntimeSystem::findRuntime(entt::entity entity) {
        auto it = m_runtimes.find(entity);
        if (it == m_runtimes.end()) {
            return nullptr;
        }
        return &it->second;
    }

    void LuaRuntimeSystem::destroyRuntime(entt::entity entity) {
        m_runtimes.erase(entity);
    }

    void LuaRuntimeSystem::applyProperties(ScriptRuntimeState &runtime,
                                           const Scene::LuaScriptComponent &component,
                                           const ScriptAsset &asset) {
        if (!runtime.properties.valid()) {
            return;
        }

        runtime.properties.clear();
        for (const auto &definition: asset.properties) {
            Scene::LuaPropertyValue value = Scene::LuaPropertyValue{};
            auto existing = component.serializedFields.find(definition.name);
            switch (definition.type) {
                case ScriptPropertyType::Boolean: {
                    bool boolValue = definition.boolDefault;
                    if (existing != component.serializedFields.end() && std::holds_alternative<
                            bool>(existing->second)) {
                        boolValue = std::get<bool>(existing->second);
                    }
                    runtime.properties[definition.name] = boolValue;
                    break;
                }
                case ScriptPropertyType::String: {
                    std::string strValue = definition.stringDefault;
                    if (existing != component.serializedFields.end() && std::holds_alternative<std::string>(
                            existing->second)) {
                        strValue = std::get<std::string>(existing->second);
                    }
                    runtime.properties[definition.name] = strValue;
                    break;
                }
                case ScriptPropertyType::Number:
                default: {
                    double numValue = definition.numberDefault;
                    if (existing != component.serializedFields.end() && std::holds_alternative<
                            double>(existing->second)) {
                        numValue = std::get<double>(existing->second);
                    }
                    runtime.properties[definition.name] = numValue;
                    break;
                }
            }
        }
    }
} // namespace Artifact::Scripting
