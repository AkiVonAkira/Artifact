#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace sol {
class state;
}

namespace Artifact::Scripting {

class LuaContext {
public:
    using ReloadCallback = std::function<void(const std::string& module)>;

    LuaContext();
    ~LuaContext();

    void executeFile(const std::filesystem::path& file);
    void executeString(const std::string& code);
    void enqueueReload(const std::string& moduleName);
    void pollReloads();
    void registerReloadCallback(ReloadCallback cb);
    sol::state& state();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::vector<std::string> m_reloadQueue;
    std::vector<ReloadCallback> m_callbacks;
};

}
