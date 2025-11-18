#pragma once

#include <string>

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

struct LuaScriptComponent {
    std::string scriptName{};
    bool autoReloadScript{true};
};

} // namespace Artifact::Scene
