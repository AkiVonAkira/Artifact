#pragma once

#include <unordered_map>
#include <string>

#include <entt/entt.hpp>
#include <sol/sol.hpp>

#include "Artifact/Scene/Components.h"
#include "Artifact/Scripting/ScriptAssetService.h"

namespace Artifact::Scripting {

class LuaBindings;
class LuaContext;

struct ScriptRuntimeState {
    std::string scriptName;
    sol::environment environment;
    sol::table definition;
    sol::table instance;
    sol::table properties;
    sol::protected_function awake;
    sol::protected_function start;
    sol::protected_function update;
    sol::protected_function onEnable;
    sol::protected_function onDisable;
    bool awakeCalled{false};
    bool startCalled{false};
    bool enabledLastFrame{false};
};

class LuaRuntimeSystem {
public:
    LuaRuntimeSystem(LuaContext& context, ScriptAssetService& assetService, LuaBindings& bindings);

    void tick(entt::registry& registry, float deltaTime, bool shouldUpdate);
    void clear();
    void onScriptAssigned(entt::entity entity, Scene::LuaScriptComponent& component);
    void onScriptPropertiesChanged(entt::entity entity, const Scene::LuaScriptComponent& component);

private:
    ScriptRuntimeState* getOrCreateRuntime(entt::entity entity, Scene::LuaScriptComponent& component);
    ScriptRuntimeState* findRuntime(entt::entity entity);
    void destroyRuntime(entt::entity entity);
    void applyProperties(ScriptRuntimeState& runtime, const Scene::LuaScriptComponent& component, const ScriptAsset& asset);

    LuaContext& m_context;
    ScriptAssetService& m_assets;
    LuaBindings& m_bindings;
    std::unordered_map<entt::entity, ScriptRuntimeState> m_runtimes;
};

} // namespace Artifact::Scripting
