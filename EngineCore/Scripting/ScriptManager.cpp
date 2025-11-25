#include "Artifact/Scripting/ScriptManager.h"

#include "Artifact/Scripting/LuaContext.h"
#include "Artifact/Scripting/LuaBindings.h"
#include "Artifact/Scripting/LuaRuntimeSystem.h"
#include "Artifact/Scripting/ScriptAssetService.h"
#include "Artifact/Scripting/ScriptAsset.h"
#include "Artifact/Core/LogManager.h"
#include "Artifact/Core/Macros.h"
#include "Artifact/Core/Config.h"

#include <sol/sol.hpp>

namespace Artifact::Scripting {
    ScriptManager::ScriptManager() {
        LOG_INFO("ScriptManager: Initializing");
    }

    ScriptManager::~ScriptManager() {
        LOG_INFO("ScriptManager: Shutting down");
        m_runtime.reset();
        m_bindings.reset();
        m_assets.reset();
        m_context.reset();
    }

    void ScriptManager::Initialize(const std::filesystem::path &rawAssetsPath) {
        if (m_initialized) {
            LOG_WARN("ScriptManager: Already initialized");
            return;
        }

        LOG_INFO("ScriptManager: Creating Lua context");
        m_context = std::make_unique<LuaContext>();

        // Register reload callback
        m_context->registerReloadCallback([](const std::string &module) {
            LOG_INFO("ScriptManager: Reloaded Lua module: {}", module);
        });

        LOG_INFO("ScriptManager: Creating bindings and runtime systems");
        m_bindings = std::make_unique<LuaBindings>(*m_context);

        // Create a temporary RuntimeConfig for ScriptAssetService
        Core::RuntimeConfig tempConfig;
        tempConfig.paths.rawAssets = rawAssetsPath;

        m_assets = std::make_unique<ScriptAssetService>(tempConfig, *m_context);
        m_runtime = std::make_unique<LuaRuntimeSystem>(*m_context, *m_assets, *m_bindings);

        m_initialized = true;
        LOG_INFO("ScriptManager: Initialization complete");
    }

    void ScriptManager::ExecuteFile(const std::filesystem::path &file) {
        if (!m_context) {
            LOG_ERROR("ScriptManager: Cannot execute file, context not initialized");
            return;
        }
        m_context->executeFile(file);
    }

    void ScriptManager::ExecuteString(const std::string &code) {
        if (!m_context) {
            LOG_ERROR("ScriptManager: Cannot execute string, context not initialized");
            return;
        }
        m_context->executeString(code);
    }

    void ScriptManager::EnqueueReload(const std::string &moduleName) {
        if (!m_context) {
            LOG_ERROR("ScriptManager: Cannot enqueue reload, context not initialized");
            return;
        }
        m_context->enqueueReload(moduleName);
    }

    void ScriptManager::PollReloads() {
        if (!m_context) {
            return;
        }
        m_context->pollReloads();
    }

    void ScriptManager::RegisterReloadCallback(ReloadCallback cb) {
        if (!m_context) {
            LOG_ERROR("ScriptManager: Cannot register reload callback, context not initialized");
            return;
        }
        m_context->registerReloadCallback(std::move(cb));
    }

    void ScriptManager::RegisterInputBindings(KeyQueryFunction keyQuery) {
        if (!m_bindings) {
            LOG_ERROR("ScriptManager: Cannot register input bindings, bindings not initialized");
            return;
        }
        m_bindings->registerInputBindings(std::move(keyQuery));
    }

    void ScriptManager::RegisterTimeBindings() {
        if (!m_bindings) {
            LOG_ERROR("ScriptManager: Cannot register time bindings, bindings not initialized");
            return;
        }
        m_bindings->registerTimeBindings();
    }

    void ScriptManager::RegisterEntityBindings() {
        if (!m_bindings) {
            LOG_ERROR("ScriptManager: Cannot register entity bindings, bindings not initialized");
            return;
        }
        m_bindings->registerEntityBindings();
    }

    void ScriptManager::SetRegistry(entt::registry *registry) {
        if (!m_bindings) {
            LOG_ERROR("ScriptManager: Cannot set registry, bindings not initialized");
            return;
        }
        m_bindings->setRegistry(registry);
    }

    void ScriptManager::SetActiveEntity(entt::entity entity) {
        if (!m_bindings) {
            LOG_ERROR("ScriptManager: Cannot set active entity, bindings not initialized");
            return;
        }
        m_bindings->setActiveEntity(entity);
    }

    void ScriptManager::ClearActiveEntity() {
        if (!m_bindings) {
            return;
        }
        m_bindings->clearActiveEntity();
    }

    void ScriptManager::UpdateDeltaTime(float deltaTime) {
        if (!m_bindings) {
            return;
        }
        m_bindings->updateDeltaTime(deltaTime);
    }

    void ScriptManager::UpdateEntityMetadata(const std::string &name, int id) {
        if (!m_bindings) {
            return;
        }
        m_bindings->updateEntityMetadata(name, id);
    }

    void ScriptManager::ClearEntityMetadata() {
        if (!m_bindings) {
            return;
        }
        m_bindings->clearEntityMetadata();
    }

    void ScriptManager::Tick(entt::registry &registry, float deltaTime, bool shouldUpdate) {
        if (!m_runtime) {
            LOG_ERROR("ScriptManager: Cannot tick, runtime not initialized");
            return;
        }
        m_runtime->tick(registry, deltaTime, shouldUpdate);
    }

    void ScriptManager::ClearRuntimes() {
        if (!m_runtime) {
            return;
        }
        m_runtime->clear();
    }

    void ScriptManager::OnScriptAssigned(entt::entity entity, Scene::LuaScriptComponent &component) {
        if (!m_runtime) {
            LOG_ERROR("ScriptManager: Cannot handle script assignment, runtime not initialized");
            return;
        }
        m_runtime->onScriptAssigned(entity, component);
    }

    void ScriptManager::OnScriptPropertiesChanged(entt::entity entity, const Scene::LuaScriptComponent &component) {
        if (!m_runtime) {
            LOG_ERROR("ScriptManager: Cannot handle properties change, runtime not initialized");
            return;
        }
        m_runtime->onScriptPropertiesChanged(entity, component);
    }

    void ScriptManager::RefreshScripts(bool includeBuiltins) {
        LOG_INFO("ScriptManager::RefreshScripts called with includeBuiltins={}", includeBuiltins);
        LOG_MANAGER().Flush();

        if (!m_assets) {
            LOG_ERROR("ScriptManager: Cannot refresh scripts, assets not initialized");
            return;
        }

        LOG_INFO("ScriptManager::RefreshScripts calling m_assets->refresh()...");
        LOG_MANAGER().Flush();
        m_assets->refresh(includeBuiltins);
        LOG_INFO("ScriptManager::RefreshScripts completed successfully");
        LOG_MANAGER().Flush();
    }

    const ScriptAsset *ScriptManager::FindScript(const std::string &name) const {
        if (!m_assets) {
            LOG_ERROR("ScriptManager: Cannot find script, assets not initialized");
            return nullptr;
        }
        return m_assets->findByName(name);
    }

    std::vector<std::string> ScriptManager::GetAvailableScripts() const {
        if (!m_assets) {
            return {};
        }

        std::vector<std::string> names;
        for (const auto &script: m_assets->scripts()) {
            names.push_back(script.name);
        }
        return names;
    }

    sol::state &ScriptManager::State() {
        if (!m_context) {
            LOG_CRITICAL("ScriptManager: Cannot get state, context not initialized");
            throw std::runtime_error("ScriptManager not initialized");
        }
        return m_context->state();
    }
} // namespace Artifact::Scripting
