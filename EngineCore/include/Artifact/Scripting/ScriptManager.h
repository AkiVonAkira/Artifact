#pragma once

#include <functional>
#include <memory>
#include <filesystem>
#include <string>

#include <entt/entt.hpp>

#include "Artifact/Core/Singleton.h"
#include "Artifact/Scene/Components.h"

namespace sol {
    class state;
}

namespace Artifact::Scripting {
    class LuaContext;
    class LuaBindings;
    class LuaRuntimeSystem;
    class ScriptAssetService;
    struct ScriptAsset;

    /**
     * @brief Central scripting system managing Lua integration
     *
     * ScriptManager is a singleton that coordinates all Lua-related functionality:
     * - Lua state management and module reloading
     * - C++ to Lua bindings registration
     * - Entity script lifecycle management
     * - Script asset loading and hot-reload
     *
     * Access via ScriptManager::Instance()
     */
    class ScriptManager : public Core::Singleton<ScriptManager> {
    public:
        using ReloadCallback = std::function<void(const std::string &module)>;
        using KeyQueryFunction = std::function<bool(int)>;

        ScriptManager();

        ~ScriptManager() override;

        // Initialization
        void Initialize(const std::filesystem::path &rawAssetsPath);

        // Lua execution
        void ExecuteFile(const std::filesystem::path &file);

        void ExecuteString(const std::string &code);

        // Module reloading
        void EnqueueReload(const std::string &moduleName);

        void PollReloads();

        void RegisterReloadCallback(ReloadCallback cb);

        // Bindings registration
        void RegisterInputBindings(KeyQueryFunction keyQuery);

        void RegisterTimeBindings();

        void RegisterEntityBindings();

        // Runtime context management
        void SetRegistry(entt::registry *registry);

        void SetActiveEntity(entt::entity entity);

        void ClearActiveEntity();

        void UpdateDeltaTime(float deltaTime);

        void UpdateEntityMetadata(const std::string &name, int id);

        void ClearEntityMetadata();

        // Entity script lifecycle
        void Tick(entt::registry &registry, float deltaTime, bool shouldUpdate);

        void ClearRuntimes();

        void OnScriptAssigned(entt::entity entity, Scene::LuaScriptComponent &component);

        void OnScriptPropertiesChanged(entt::entity entity, const Scene::LuaScriptComponent &component);

        // Asset management
        void RefreshScripts(bool includeBuiltins = true);

        const ScriptAsset *FindScript(const std::string &name) const;

        std::vector<std::string> GetAvailableScripts() const;

        // Direct access (for advanced use cases)
        sol::state &State();

        LuaContext *GetContext() { return m_context.get(); }
        LuaBindings *GetBindings() { return m_bindings.get(); }
        LuaRuntimeSystem *GetRuntimeSystem() { return m_runtime.get(); }
        ScriptAssetService *GetAssetService() { return m_assets.get(); }

    private:
        std::unique_ptr<LuaContext> m_context;
        std::unique_ptr<LuaBindings> m_bindings;
        std::unique_ptr<LuaRuntimeSystem> m_runtime;
        std::unique_ptr<ScriptAssetService> m_assets;

        bool m_initialized{false};
    };
} // namespace Artifact::Scripting
