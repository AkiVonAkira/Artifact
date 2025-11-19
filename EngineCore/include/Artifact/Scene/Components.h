#pragma once

#include <string>
#include <unordered_map>
#include <variant>

#include <glm/vec3.hpp>
#include <entt/entt.hpp>

#include "Artifact/Rendering/Camera.h"

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
    entt::entity parent{entt::null};
    entt::entity firstChild{entt::null};
    entt::entity nextSibling{entt::null};
    entt::entity previousSibling{entt::null};
};

struct CameraComponent {
    Rendering::Camera camera{};
    bool isPrimary{true};
    int renderOrder{0};
    uint32_t cullingMask{0xFFFFFFFFu};
    bool enabled{true};
};

using LuaPropertyValue = std::variant<double, bool, std::string>;

struct LuaScriptComponent {
    std::string scriptName{};
    bool enabled{true};
    bool autoReloadScript{true};
    std::unordered_map<std::string, LuaPropertyValue> serializedFields;
};

} // namespace Artifact::Scene
