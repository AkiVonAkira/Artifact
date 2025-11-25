#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

#include "Artifact/Core/Config.h"
#include "Artifact/Scripting/ScriptAsset.h"

namespace Artifact::Scripting {
    class LuaContext;

    class ScriptAssetService {
    public:
        using ChangeCallback = std::function<void()>;

        ScriptAssetService(const Core::RuntimeConfig &runtimeConfig, LuaContext &context);

        void refresh(bool announce = true);

        const std::vector<ScriptAsset> &scripts() const;

        const ScriptAsset *findByName(std::string_view name) const;

        bool createScript(const std::string &scriptName, std::string &outError);

        void onChanged(ChangeCallback callback);

    private:
        void ensureScriptsFolder() const;

        ScriptAsset parseAsset(const std::filesystem::path &path) const;

        Core::RuntimeConfig m_config;
        LuaContext &m_context;
        std::vector<ScriptAsset> m_scripts;
        ChangeCallback m_callback;
    };
} // namespace Artifact::Scripting
