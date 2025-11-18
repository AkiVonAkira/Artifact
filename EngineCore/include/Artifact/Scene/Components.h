#pragma once

#include <string>
#include <unordered_map>
#include <variant>

#include <glm/vec3.hpp>

namespace Artifact::Scene {

struct Transform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

struct IdComponent {
    int id{0};
};

struct NameComponent {
    std::string name{"GameObject"};
};

struct ActiveComponent {
    bool active{true};
};

struct TransformComponent {
    Transform transform{};
};

using LuaPropertyValue = std::variant<double, bool, std::string>;

struct LuaScriptComponent {
    std::string scriptName{};
    bool enabled{true};
    bool autoReloadScript{true};
    std::unordered_map<std::string, LuaPropertyValue> serializedFields;
};

} // namespace Artifact::Scene
