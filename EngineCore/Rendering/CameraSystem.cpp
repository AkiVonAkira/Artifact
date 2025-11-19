#include "Artifact/Rendering/CameraSystem.h"

namespace Artifact::Rendering {

Camera* CameraSystem::getPrimaryCamera(entt::registry& registry) {
    Camera* firstCamera = nullptr;

    auto view = registry.view<Scene::CameraComponent>();
    for (auto entity : view) {
        auto& camComp = view.get<Scene::CameraComponent>(entity);
        if (!firstCamera) {
            firstCamera = &camComp.camera;
        }
        if (camComp.isPrimary) {
            return &camComp.camera;
        }
    }

    return firstCamera;
}

} // namespace Artifact::Rendering

