#pragma once
#include <entt/entity/registry.hpp>

namespace Artifact::Scene {
    struct SceneContext;
}

namespace Artifact::Rendering {
    class Camera;
}

class Renderer {
public:
// ...existing declarations...
    void renderScene(const Artifact::Scene::SceneContext& scene, Artifact::Rendering::Camera* overrideCamera = nullptr);

Artifact::Rendering::Camera* bindRuntimeCamera(entt::registry& registry);
// ...existing code...
};
