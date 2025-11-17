#include "Artifact/Scripting/LuaContext.h"

#include "Artifact/Core/Instrumentation.h"

#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

namespace Artifact::Scripting {

struct LuaContext::Impl {
    sol::state state;

    Impl() {
        state.open_libraries(sol::lib::base, sol::lib::math, sol::lib::package, sol::lib::table, sol::lib::string);
    }
};

LuaContext::LuaContext()
    : m_impl(std::make_unique<Impl>()) {}

LuaContext::~LuaContext() = default;

void LuaContext::executeFile(const std::filesystem::path& file) {
    ARTIFACT_ZONE();
    auto result = m_impl->state.safe_script_file(file.string(), sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        spdlog::error("Lua error in {}: {}", file.string(), err.what());
    }
}

void LuaContext::executeString(const std::string& code) {
    ARTIFACT_ZONE();
    auto result = m_impl->state.safe_script(code, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        spdlog::error("Lua error: {}", err.what());
    }
}

void LuaContext::enqueueReload(const std::string& moduleName) {
    ARTIFACT_ZONE();
    m_reloadQueue.push_back(moduleName);
}

void LuaContext::pollReloads() {
    ARTIFACT_ZONE();
    for (const auto& module : m_reloadQueue) {
        spdlog::info("Reloading module {}", module);
        const std::string command = "package.loaded['" + module + "']=nil; require('" + module + "')";
        m_impl->state.script(command);
        for (auto& cb : m_callbacks) {
            cb(module);
        }
    }
    m_reloadQueue.clear();
}

void LuaContext::registerReloadCallback(ReloadCallback cb) {
    m_callbacks.emplace_back(std::move(cb));
}

}
