#pragma once

namespace Artifact::Scene {
    struct SceneContext;
}

namespace Artifact::Rendering {
    class Camera;
}

class Renderer {
public:
// ...existing declarations...
    void renderScene(const Artifact::Scene::SceneContext& scene, Rendering::Camera* overrideCamera = nullptr);

Artifact::Rendering::Camera* bindRuntimeCamera(entt::registry& registry);
// ...existing code...
};
