#include "Artifact/Scripting/ScriptAssetService.h"

#include <algorithm>
#include <fstream>
#include <sol/sol.hpp>

#include "Artifact/Core/Macros.h"
#include "Artifact/Core/Paths.h"
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

    ScriptAssetService::ScriptAssetService(const Core::RuntimeConfig &runtimeConfig, LuaContext &context)
        : m_config(runtimeConfig)
          , m_context(context) {
    }

    void ScriptAssetService::refresh(bool announce) {
        try {
            LOG_INFO("[ScriptAssetService] refresh() called, announce={}", announce);

            ensureScriptsFolder();
            LOG_INFO("[ScriptAssetService] ensureScriptsFolder() completed");

            m_scripts.clear();
            LOG_INFO("[ScriptAssetService] m_scripts cleared");

            const auto scriptsRoot = m_config.paths.rawAssets / "Scripts";
            LOG_INFO("[ScriptAssetService] scriptsRoot = {}", scriptsRoot.string());

            // Check if the directory exists before trying to iterate
            std::error_code ec;
            if (!std::filesystem::exists(scriptsRoot, ec)) {
                LOG_WARN("[ScriptAssetService] Scripts directory does not exist: {}", scriptsRoot.string());
                if (announce && m_callback) {
                    m_callback();
                }
                return;
            }

            LOG_INFO("[ScriptAssetService] Scripts directory exists, creating iterator...");

            // Use error_code version to avoid exceptions/aborts
            std::filesystem::recursive_directory_iterator dirIter(scriptsRoot, ec);
            if (ec) {
                LOG_ERROR("[ScriptAssetService] Failed to open scripts directory '{}': {}", scriptsRoot.string(), ec.message());
                if (announce && m_callback) {
                    m_callback();
                }
                return;
            }

            LOG_INFO("[ScriptAssetService] Directory iterator created, scanning for .lua files...");

            int fileCount = 0;
            for (const auto &entry: dirIter) {
                if (!entry.is_regular_file(ec) || ec || entry.path().extension() != ".lua") {
                    if (ec) {
                        LOG_WARN("[ScriptAssetService] Error checking file '{}': {}", entry.path().string(), ec.message());
                        ec.clear();
                    }
                    continue;
                }

                LOG_INFO("[ScriptAssetService] Parsing script file: {}", entry.path().string());

                m_scripts.emplace_back(parseAsset(entry.path()));
                fileCount++;

                LOG_INFO("[ScriptAssetService] Successfully parsed script #{}", fileCount);
            }

            LOG_INFO("[ScriptAssetService] Found {} script files, sorting...", fileCount);

            std::sort(m_scripts.begin(), m_scripts.end(), [](const ScriptAsset &a, const ScriptAsset &b) {
                return a.name < b.name;
            });

            LOG_INFO("[ScriptAssetService] Scripts sorted successfully");

            if (announce && m_callback) {
                LOG_INFO("[ScriptAssetService] Calling callback...");
                m_callback();
            }

            LOG_INFO("[ScriptAssetService] refresh() completed successfully");
        } catch (const std::exception &ex) {
            LOG_ERROR("[ScriptAssetService] Exception during refresh: {}", ex.what());
            if (announce && m_callback) {
                m_callback();
            }
        } catch (...) {
            LOG_ERROR("[ScriptAssetService] Unknown exception during refresh");
            if (announce && m_callback) {
                m_callback();
            }
        }
    }

    const std::vector<ScriptAsset> &ScriptAssetService::scripts() const {
        return m_scripts;
    }

    const ScriptAsset *ScriptAssetService::findByName(std::string_view name) const {
        auto it = std::find_if(m_scripts.begin(), m_scripts.end(), [&name](const ScriptAsset &asset) {
            return asset.name == name;
        });
        if (it == m_scripts.end()) {
            return nullptr;
        }
        return &(*it);
    }

    bool ScriptAssetService::createScript(const std::string &scriptName, std::string &outError) {
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
        LOG_INFO("[ScriptAssetService] ensureScriptsFolder() start");

        // Determine scripts root, falling back to project Assets/Raw if config is empty
        std::filesystem::path scriptsRoot = m_config.paths.rawAssets;
        if (scriptsRoot.empty()) {
            LOG_WARN("[ScriptAssetService] m_config.paths.rawAssets is empty, falling back to default Assets/Raw");
            scriptsRoot = Core::Paths::AssetsRoot() / "Raw";
        }

        LOG_INFO("[ScriptAssetService] rawAssets path: {}", scriptsRoot.string());

        std::error_code ec;
        try {
            std::filesystem::create_directories(scriptsRoot, ec);
        } catch (const std::exception &ex) {
            LOG_ERROR("[ScriptAssetService] Exception while creating scripts directory '{}': {}", scriptsRoot.string(), ex.what());
            return;
        } catch (...) {
            LOG_ERROR("[ScriptAssetService] Unknown exception while creating scripts directory '{}'", scriptsRoot.string());
            return;
        }

        if (ec) {
            LOG_ERROR("[ScriptAssetService] Failed to create scripts directory '{}': {}", scriptsRoot.string(), ec.message());
        } else {
            LOG_INFO("[ScriptAssetService] ensureScriptsFolder() completed");
        }
    }

    ScriptAsset ScriptAssetService::parseAsset(const std::filesystem::path &path) const {
        LOG_INFO("[ScriptAssetService] parseAsset() start for: {}", path.string());

        ScriptAsset asset;
        asset.name = path.stem().string();
        asset.path = std::filesystem::absolute(path);

        LOG_INFO("[ScriptAssetService] Getting Lua state...");

        auto &lua = m_context.state();

        LOG_INFO("[ScriptAssetService] Creating environment...");

        sol::environment env(lua, sol::create, lua.globals());

        LOG_INFO("[ScriptAssetService] Executing script file...");

        sol::protected_function_result execResult = lua.safe_script_file(path.string(), env, sol::script_pass_on_error);

        LOG_INFO("[ScriptAssetService] Script executed, checking result...");

        if (!execResult.valid()) {
            sol::error err = execResult;
            LOG_WARN("[Editor] Failed to parse Lua script '{}': {}", path.string(), err.what());
            return asset;
        }

        LOG_INFO("[ScriptAssetService] Getting definition object...");

        sol::object definitionObject;
        if (execResult.return_count() > 0) {
            definitionObject = execResult.get<sol::object>();
        }

        LOG_INFO("[ScriptAssetService] Creating definition table...");

        sol::table definitionTable;
        if (definitionObject.valid() && definitionObject.get_type() == sol::type::table) {
            definitionTable = definitionObject;
        } else {
            definitionTable = env;
        }

        LOG_INFO("[ScriptAssetService] Getting properties object...");

        sol::object propertiesObject = definitionTable["properties"];
        if (!propertiesObject.valid() || propertiesObject.get_type() != sol::type::table) {
            LOG_INFO("[ScriptAssetService] No properties table found, returning asset");
            return asset;
        }

        LOG_INFO("[ScriptAssetService] Parsing properties table...");

        sol::table propertiesTable = propertiesObject;
        for (auto &&pair: propertiesTable) {
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
