#include "Artifact/Scripting/ScriptAssetService.h"

#include <algorithm>
#include <cctype>
#include <fstream>

#include <spdlog/spdlog.h>
#include <sol/sol.hpp>

#include "Artifact/Scripting/LuaContext.h"

namespace Artifact::Scripting {

namespace {

ScriptPropertyType ParseType(std::string_view label) {
    std::string lowered(label.begin(), label.end());
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (lowered == "bool" || lowered == "boolean") {
        return ScriptPropertyType::Boolean;
    }
    if (lowered == "string" || lowered == "text") {
        return ScriptPropertyType::String;
    }
    return ScriptPropertyType::Number;
}

} // namespace

ScriptAssetService::ScriptAssetService(const Core::RuntimeConfig& runtimeConfig, LuaContext& context)
    : m_config(runtimeConfig)
    , m_context(context) {}

void ScriptAssetService::refresh(bool announce) {
    ensureScriptsFolder();
    m_scripts.clear();

    const auto scriptsRoot = m_config.paths.rawAssets / "Scripts";
    for (const auto& entry : std::filesystem::recursive_directory_iterator(scriptsRoot)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") {
            continue;
        }
        m_scripts.emplace_back(parseAsset(entry.path()));
    }

    std::sort(m_scripts.begin(), m_scripts.end(), [](const ScriptAsset& a, const ScriptAsset& b) {
        return a.name < b.name;
    });

    if (announce && m_callback) {
        m_callback();
    }
}

const std::vector<ScriptAsset>& ScriptAssetService::scripts() const {
    return m_scripts;
}

const ScriptAsset* ScriptAssetService::findByName(std::string_view name) const {
    auto it = std::find_if(m_scripts.begin(), m_scripts.end(), [&name](const ScriptAsset& asset) {
        return asset.name == name;
    });
    if (it == m_scripts.end()) {
        return nullptr;
    }
    return &(*it);
}

bool ScriptAssetService::createScript(const std::string& scriptName, std::string& outError) {
    if (scriptName.empty()) {
        outError = "Invalid script name";
        return false;
    }

    ensureScriptsFolder();
    const auto scriptsRoot = m_config.paths.rawAssets / "Scripts";
    const auto newScriptPath = scriptsRoot / (scriptName + ".lua");
    if (std::filesystem::exists(newScriptPath)) {
        outError = "Script already exists";
        return false;
    }

    std::ofstream file(newScriptPath);
    if (!file.is_open()) {
        outError = "Failed to create script";
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

    refresh(false);
    if (m_callback) {
        m_callback();
    }
    return true;
}

void ScriptAssetService::onChanged(ChangeCallback callback) {
    m_callback = std::move(callback);
}

void ScriptAssetService::ensureScriptsFolder() const {
    const auto scriptsRoot = m_config.paths.rawAssets / "Scripts";
    std::error_code ec;
    std::filesystem::create_directories(scriptsRoot, ec);
}

ScriptAsset ScriptAssetService::parseAsset(const std::filesystem::path& path) const {
    ScriptAsset asset;
    asset.name = path.stem().string();
    asset.path = std::filesystem::absolute(path);

    auto& lua = m_context.state();
    sol::environment env(lua, sol::create, lua.globals());
    sol::protected_function_result execResult = lua.safe_script_file(path.string(), env, sol::script_pass_on_error);
    if (!execResult.valid()) {
        sol::error err = execResult;
        spdlog::warn("[Editor] Failed to parse Lua script '{}': {}", path.string(), err.what());
        return asset;
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
        return asset;
    }

    sol::table propertiesTable = propertiesObject;
    for (auto&& pair : propertiesTable) {
        sol::object key = pair.first;
        sol::object value = pair.second;
        if (key.get_type() != sol::type::string || value.get_type() != sol::type::table) {
            continue;
        }

        ScriptPropertyDefinition definition;
        definition.name = key.as<std::string>();

        sol::table propertyTable = value;
        definition.type = ParseType(propertyTable.get_or("type", std::string{"float"}));

        switch (definition.type) {
            case ScriptPropertyType::Boolean: {
                sol::object defaultObject = propertyTable.raw_get<sol::object>("default");
                if (defaultObject.valid() && defaultObject.get_type() == sol::type::boolean) {
                    definition.boolDefault = defaultObject.as<bool>();
                } else {
                    definition.boolDefault = true;
                }
                break;
            }
            case ScriptPropertyType::String: {
                sol::object defaultObject = propertyTable.raw_get<sol::object>("default");
                if (defaultObject.valid() && defaultObject.get_type() == sol::type::string) {
                    definition.stringDefault = defaultObject.as<std::string>();
                }
                break;
            }
            case ScriptPropertyType::Number:
            default: {
                sol::object defaultObject = propertyTable.raw_get<sol::object>("default");
                if (defaultObject.valid() && defaultObject.get_type() == sol::type::number) {
                    definition.numberDefault = defaultObject.as<double>();
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

        asset.properties.emplace_back(std::move(definition));
    }

    return asset;
}

} // namespace Artifact::Scripting

