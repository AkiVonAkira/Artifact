#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Artifact::Scripting {

enum class ScriptPropertyType {
    Number,
    Boolean,
    String
};

struct ScriptPropertyDefinition {
    std::string name;
    ScriptPropertyType type{ScriptPropertyType::Number};
    double numberDefault{0.0};
    bool boolDefault{false};
    std::string stringDefault{};
    std::optional<double> minValue{};
    std::optional<double> maxValue{};
};

struct ScriptAsset {
    std::string name;
    std::filesystem::path path;
    std::vector<ScriptPropertyDefinition> properties;
};

} // namespace Artifact::Scripting

