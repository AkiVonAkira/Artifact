# C++ Camera System Implementation Guide

**Priority:** High (Phase 1)  
**Estimated Effort:** 3-5 days  
**Dependencies:** None

---

## Overview

This document provides a step-by-step guide to implement a production-ready camera system in C++, replacing the current Lua-based `CameraController.lua`.

---

## Design Goals

1. **Performance**: Sub-millisecond update times for camera transforms
2. **Flexibility**: Support both 3D (perspective) and 2D (orthographic) cameras
3. **Editor Integration**: Seamless integration with ImGui viewport
4. **Lua Compatibility**: Allow Lua scripts to control camera for prototyping (optional API)
5. **Frustum Culling**: Built-in visibility testing for optimization

---

## Step 1: Core Camera Class

### File: `EngineCore/include/Artifact/Rendering/Camera.h`

```cpp
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Artifact::Rendering {

enum class ProjectionType {
    Perspective,
    Orthographic
};

struct Frustum {
    glm::vec4 planes[6]; // Left, Right, Bottom, Top, Near, Far
    
    bool contains(const glm::vec3& point) const;
    bool intersects(const glm::vec3& center, float radius) const;
};

class Camera {
public:
    explicit Camera(ProjectionType type = ProjectionType::Perspective);
    
    // Transform manipulation
    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void setRotation(const glm::vec3& eulerAngles); // Convenience (degrees)
    void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Projection configuration
    void setPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane);
    void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
    void setOrthographicSize(float size, float aspectRatio, float nearPlane, float farPlane);
    
    // Getters
    const glm::vec3& position() const { return m_position; }
    const glm::quat& rotation() const { return m_rotation; }
    glm::vec3 eulerAngles() const; // In degrees
    
    glm::vec3 forward() const;
    glm::vec3 right() const;
    glm::vec3 up() const;
    
    const glm::mat4& viewMatrix() const;
    const glm::mat4& projectionMatrix() const;
    glm::mat4 viewProjectionMatrix() const;
    
    const Frustum& frustum() const;
    
    // Projection parameters
    ProjectionType projectionType() const { return m_projectionType; }
    float fieldOfView() const { return m_fov; }
    float aspectRatio() const { return m_aspectRatio; }
    float nearPlane() const { return m_nearPlane; }
    float farPlane() const { return m_farPlane; }
    
    // Screen/World conversions
    glm::vec3 screenToWorldPoint(const glm::vec2& screenPos, float depth, const glm::vec2& viewportSize) const;
    glm::vec2 worldToScreenPoint(const glm::vec3& worldPos, const glm::vec2& viewportSize) const;
    
    // Ray casting
    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;
    };
    Ray screenPointToRay(const glm::vec2& screenPos, const glm::vec2& viewportSize) const;
    
private:
    void updateViewMatrix() const;
    void updateProjectionMatrix() const;
    void updateFrustum() const;
    
    // Transform
    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
    glm::quat m_rotation{1.0f, 0.0f, 0.0f, 0.0f}; // Identity (w, x, y, z)
    
    // Projection
    ProjectionType m_projectionType;
    float m_fov{45.0f};          // Field of view (degrees)
    float m_aspectRatio{16.0f / 9.0f};
    float m_nearPlane{0.1f};
    float m_farPlane{1000.0f};
    
    // Orthographic params
    float m_orthoLeft{-10.0f};
    float m_orthoRight{10.0f};
    float m_orthoBottom{-10.0f};
    float m_orthoTop{10.0f};
    
    // Cached matrices (lazy evaluation)
    mutable glm::mat4 m_viewMatrix{1.0f};
    mutable glm::mat4 m_projectionMatrix{1.0f};
    mutable Frustum m_frustum{};
    mutable bool m_viewDirty{true};
    mutable bool m_projectionDirty{true};
    mutable bool m_frustumDirty{true};
};

} // namespace Artifact::Rendering
```

---

### File: `EngineCore/Rendering/Camera.cpp`

```cpp
#include "Artifact/Rendering/Camera.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace Artifact::Rendering {

Camera::Camera(ProjectionType type)
    : m_projectionType(type) {
    if (type == ProjectionType::Perspective) {
        setPerspective(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    } else {
        setOrthographic(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
    }
}

void Camera::setPosition(const glm::vec3& position) {
    m_position = position;
    m_viewDirty = true;
    m_frustumDirty = true;
}

void Camera::setRotation(const glm::quat& rotation) {
    m_rotation = glm::normalize(rotation);
    m_viewDirty = true;
    m_frustumDirty = true;
}

void Camera::setRotation(const glm::vec3& eulerAngles) {
    // Convert Euler angles (degrees) to quaternion
    glm::vec3 radians = glm::radians(eulerAngles);
    m_rotation = glm::quat(radians);
    m_viewDirty = true;
    m_frustumDirty = true;
}

void Camera::lookAt(const glm::vec3& target, const glm::vec3& up) {
    glm::vec3 direction = glm::normalize(target - m_position);
    
    // Calculate rotation quaternion from direction
    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    glm::vec3 actualUp = glm::cross(right, direction);
    
    glm::mat3 rotationMatrix;
    rotationMatrix[0] = right;
    rotationMatrix[1] = actualUp;
    rotationMatrix[2] = -direction; // OpenGL uses -Z as forward
    
    m_rotation = glm::quat_cast(rotationMatrix);
    m_viewDirty = true;
    m_frustumDirty = true;
}

void Camera::setPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane) {
    m_projectionType = ProjectionType::Perspective;
    m_fov = fovDegrees;
    m_aspectRatio = aspectRatio;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_projectionDirty = true;
    m_frustumDirty = true;
}

void Camera::setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
    m_projectionType = ProjectionType::Orthographic;
    m_orthoLeft = left;
    m_orthoRight = right;
    m_orthoBottom = bottom;
    m_orthoTop = top;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_projectionDirty = true;
    m_frustumDirty = true;
}

void Camera::setOrthographicSize(float size, float aspectRatio, float nearPlane, float farPlane) {
    float halfHeight = size;
    float halfWidth = halfHeight * aspectRatio;
    setOrthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
}

glm::vec3 Camera::eulerAngles() const {
    return glm::degrees(glm::eulerAngles(m_rotation));
}

glm::vec3 Camera::forward() const {
    return glm::normalize(m_rotation * glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 Camera::right() const {
    return glm::normalize(m_rotation * glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Camera::up() const {
    return glm::normalize(m_rotation * glm::vec3(0.0f, 1.0f, 0.0f));
}

const glm::mat4& Camera::viewMatrix() const {
    if (m_viewDirty) {
        updateViewMatrix();
        m_viewDirty = false;
    }
    return m_viewMatrix;
}

const glm::mat4& Camera::projectionMatrix() const {
    if (m_projectionDirty) {
        updateProjectionMatrix();
        m_projectionDirty = false;
    }
    return m_projectionMatrix;
}

glm::mat4 Camera::viewProjectionMatrix() const {
    return projectionMatrix() * viewMatrix();
}

const Frustum& Camera::frustum() const {
    if (m_frustumDirty) {
        updateFrustum();
        m_frustumDirty = false;
    }
    return m_frustum;
}

Camera::Ray Camera::screenPointToRay(const glm::vec2& screenPos, const glm::vec2& viewportSize) const {
    // Convert screen coordinates to NDC [-1, 1]
    glm::vec2 ndc = (screenPos / viewportSize) * 2.0f - 1.0f;
    ndc.y = -ndc.y; // Flip Y (screen origin is top-left)
    
    // Ray in clip space
    glm::vec4 rayClip(ndc.x, ndc.y, -1.0f, 1.0f);
    
    // Ray in eye/view space
    glm::vec4 rayEye = glm::inverse(projectionMatrix()) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    
    // Ray in world space
    glm::vec3 rayWorld = glm::vec3(glm::inverse(viewMatrix()) * rayEye);
    rayWorld = glm::normalize(rayWorld);
    
    return Ray{m_position, rayWorld};
}

void Camera::updateViewMatrix() const {
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), -m_position);
    glm::mat4 rotation = glm::mat4_cast(glm::conjugate(m_rotation));
    m_viewMatrix = rotation * translation;
}

void Camera::updateProjectionMatrix() const {
    if (m_projectionType == ProjectionType::Perspective) {
        m_projectionMatrix = glm::perspective(
            glm::radians(m_fov),
            m_aspectRatio,
            m_nearPlane,
            m_farPlane
        );
    } else {
        m_projectionMatrix = glm::ortho(
            m_orthoLeft, m_orthoRight,
            m_orthoBottom, m_orthoTop,
            m_nearPlane, m_farPlane
        );
    }
}

void Camera::updateFrustum() const {
    glm::mat4 vp = viewProjectionMatrix();
    
    // Extract frustum planes from view-projection matrix
    // Left plane
    m_frustum.planes[0] = glm::vec4(
        vp[0][3] + vp[0][0],
        vp[1][3] + vp[1][0],
        vp[2][3] + vp[2][0],
        vp[3][3] + vp[3][0]
    );
    
    // Right plane
    m_frustum.planes[1] = glm::vec4(
        vp[0][3] - vp[0][0],
        vp[1][3] - vp[1][0],
        vp[2][3] - vp[2][0],
        vp[3][3] - vp[3][0]
    );
    
    // Bottom plane
    m_frustum.planes[2] = glm::vec4(
        vp[0][3] + vp[0][1],
        vp[1][3] + vp[1][1],
        vp[2][3] + vp[2][1],
        vp[3][3] + vp[3][1]
    );
    
    // Top plane
    m_frustum.planes[3] = glm::vec4(
        vp[0][3] - vp[0][1],
        vp[1][3] - vp[1][1],
        vp[2][3] - vp[2][1],
        vp[3][3] - vp[3][1]
    );
    
    // Near plane
    m_frustum.planes[4] = glm::vec4(
        vp[0][3] + vp[0][2],
        vp[1][3] + vp[1][2],
        vp[2][3] + vp[2][2],
        vp[3][3] + vp[3][2]
    );
    
    // Far plane
    m_frustum.planes[5] = glm::vec4(
        vp[0][3] - vp[0][2],
        vp[1][3] - vp[1][2],
        vp[2][3] - vp[2][2],
        vp[3][3] - vp[3][2]
    );
    
    // Normalize planes
    for (int i = 0; i < 6; ++i) {
        float length = glm::length(glm::vec3(m_frustum.planes[i]));
        m_frustum.planes[i] /= length;
    }
}

bool Frustum::contains(const glm::vec3& point) const {
    for (int i = 0; i < 6; ++i) {
        float distance = glm::dot(glm::vec3(planes[i]), point) + planes[i].w;
        if (distance < 0.0f) {
            return false;
        }
    }
    return true;
}

bool Frustum::intersects(const glm::vec3& center, float radius) const {
    for (int i = 0; i < 6; ++i) {
        float distance = glm::dot(glm::vec3(planes[i]), center) + planes[i].w;
        if (distance < -radius) {
            return false;
        }
    }
    return true;
}

} // namespace Artifact::Rendering
```

---

## Step 2: ECS Component

### Update: `EngineCore/include/Artifact/Scene/Components.h`

```cpp
#include "Artifact/Rendering/Camera.h"

namespace Artifact::Scene {

struct CameraComponent {
    Rendering::Camera camera;
    
    bool isPrimary{true};         // Is this the main scene camera?
    int renderOrder{0};           // For multi-camera rendering
    uint32_t cullingMask{0xFFFFFFFF}; // Layer-based culling (all layers by default)
    
    // Viewport (if different from window size)
    bool useCustomViewport{false};
    float viewportX{0.0f};        // Normalized [0, 1]
    float viewportY{0.0f};
    float viewportWidth{1.0f};
    float viewportHeight{1.0f};
};

} // namespace Artifact::Scene
```

---

## Step 3: Camera System

### File: `EngineCore/include/Artifact/Rendering/CameraSystem.h`

```cpp
#pragma once

#include <entt/entt.hpp>
#include "Artifact/Rendering/Camera.h"

namespace Artifact::Scene {
struct CameraComponent;
}

namespace Artifact::Rendering {

class CameraSystem {
public:
    // Find and return the primary camera in the scene
    static Camera* getPrimaryCamera(entt::registry& registry);
    static const Camera* getPrimaryCamera(const entt::registry& registry);
    
    // Get camera component by entity
    static Scene::CameraComponent* getCameraComponent(entt::registry& registry, entt::entity entity);
    
    // Update camera aspect ratios based on viewport size
    static void updateAspectRatios(entt::registry& registry, float width, float height);
    
    // Multi-camera rendering (sorted by renderOrder)
    static std::vector<entt::entity> getSortedCameras(entt::registry& registry);
};

} // namespace Artifact::Rendering
```

### File: `EngineCore/Rendering/CameraSystem.cpp`

```cpp
#include "Artifact/Rendering/CameraSystem.h"
#include "Artifact/Scene/Components.h"
#include <algorithm>

namespace Artifact::Rendering {

Camera* CameraSystem::getPrimaryCamera(entt::registry& registry) {
    auto view = registry.view<Scene::CameraComponent>();
    for (auto entity : view) {
        auto& camComp = view.get<Scene::CameraComponent>(entity);
        if (camComp.isPrimary) {
            return &camComp.camera;
        }
    }
    return nullptr;
}

const Camera* CameraSystem::getPrimaryCamera(const entt::registry& registry) {
    auto view = registry.view<const Scene::CameraComponent>();
    for (auto entity : view) {
        const auto& camComp = view.get<const Scene::CameraComponent>(entity);
        if (camComp.isPrimary) {
            return &camComp.camera;
        }
    }
    return nullptr;
}

Scene::CameraComponent* CameraSystem::getCameraComponent(entt::registry& registry, entt::entity entity) {
    if (registry.all_of<Scene::CameraComponent>(entity)) {
        return &registry.get<Scene::CameraComponent>(entity);
    }
    return nullptr;
}

void CameraSystem::updateAspectRatios(entt::registry& registry, float width, float height) {
    if (width <= 0.0f || height <= 0.0f) return;
    
    float aspectRatio = width / height;
    auto view = registry.view<Scene::CameraComponent>();
    for (auto entity : view) {
        auto& camComp = view.get<Scene::CameraComponent>(entity);
        auto& cam = camComp.camera;
        
        if (cam.projectionType() == ProjectionType::Perspective) {
            cam.setPerspective(cam.fieldOfView(), aspectRatio, cam.nearPlane(), cam.farPlane());
        } else {
            // Update orthographic aspect ratio
            float orthoHeight = (cam.projectionMatrix()[1][1] != 0.0f) ? 
                (2.0f / cam.projectionMatrix()[1][1]) : 10.0f;
            cam.setOrthographicSize(orthoHeight, aspectRatio, cam.nearPlane(), cam.farPlane());
        }
    }
}

std::vector<entt::entity> CameraSystem::getSortedCameras(entt::registry& registry) {
    std::vector<entt::entity> cameras;
    auto view = registry.view<Scene::CameraComponent>();
    
    for (auto entity : view) {
        cameras.push_back(entity);
    }
    
    std::sort(cameras.begin(), cameras.end(), [&](entt::entity a, entt::entity b) {
        const auto& camA = registry.get<Scene::CameraComponent>(a);
        const auto& camB = registry.get<Scene::CameraComponent>(b);
        return camA.renderOrder < camB.renderOrder;
    });
    
    return cameras;
}

} // namespace Artifact::Rendering
```

---

## Step 4: Integration with Renderer

### Update: `EngineCore/Rendering/Renderer.cpp`

```cpp
#include "Artifact/Rendering/CameraSystem.h"

void Renderer::renderScene(entt::registry& registry) {
    Camera* primaryCam = CameraSystem::getPrimaryCamera(registry);
    if (!primaryCam) {
        return; // No camera in scene
    }
    
    // Update frustum culling
    const Frustum& frustum = primaryCam->frustum();
    
    // Query all renderable entities
    auto view = registry.view<Scene::TransformComponent, Scene::MeshRendererComponent>();
    for (auto entity : view) {
        const auto& transform = view.get<Scene::TransformComponent>(entity);
        const auto& renderer = view.get<Scene::MeshRendererComponent>(entity);
        
        // Frustum culling (if mesh has bounds)
        if (renderer.mesh && renderer.mesh->hasBounds()) {
            const auto& bounds = renderer.mesh->bounds();
            glm::vec3 worldCenter = transform.transform.position + bounds.center;
            if (!frustum.intersects(worldCenter, bounds.radius)) {
                continue; // Outside frustum
            }
        }
        
        // Submit to render queue
        RenderCommand cmd;
        cmd.mesh = renderer.mesh;
        cmd.material = renderer.material;
        cmd.transform = transform.getWorldMatrix();
        m_renderQueue.submit(cmd);
    }
    
    // Sort and execute
    m_renderQueue.sort(*primaryCam);
    m_renderQueue.execute(*primaryCam);
    m_renderQueue.clear();
}
```

---

## Step 5: Lua Bindings (Optional)

### Update: `EngineCore/Scripting/LuaBindings.cpp`

```cpp
void LuaBindings::registerCameraBindings() {
    auto& lua = m_context.state();
    
    // Camera type
    sol::table camera = lua.create_table();
    lua["Camera"] = camera;
    
    camera.set_function("GetPrimary", [this]() {
        if (!m_registry) return sol::nil;
        Camera* cam = CameraSystem::getPrimaryCamera(*m_registry);
        return cam ? sol::make_object(lua, cam) : sol::nil;
    });
    
    // Camera methods (if entity has CameraComponent)
    camera.set_function("SetPosition", [this](float x, float y, float z) {
        if (!m_registry || m_activeEntity == entt::null) return;
        auto* camComp = CameraSystem::getCameraComponent(*m_registry, m_activeEntity);
        if (camComp) {
            camComp->camera.setPosition({x, y, z});
        }
    });
    
    camera.set_function("LookAt", [this](float targetX, float targetY, float targetZ) {
        if (!m_registry || m_activeEntity == entt::null) return;
        auto* camComp = CameraSystem::getCameraComponent(*m_registry, m_activeEntity);
        if (camComp) {
            camComp->camera.lookAt({targetX, targetY, targetZ});
        }
    });
    
    camera.set_function("GetPosition", [this]() {
        if (!m_registry || m_activeEntity == entt::null) return lua.create_table();
        auto* camComp = CameraSystem::getCameraComponent(*m_registry, m_activeEntity);
        if (camComp) {
            const auto& pos = camComp->camera.position();
            return lua.create_table_with("x", pos.x, "y", pos.y, "z", pos.z);
        }
        return lua.create_table();
    });
}
```

---

## Step 6: Editor Integration

### Update: `Editor/Panels/InspectorPanel.cpp`

```cpp
void InspectorPanel::drawCameraComponent(Scene::CameraComponent& camera) {
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Primary", &camera.isPrimary);
        ImGui::DragInt("Render Order", &camera.renderOrder);
        
        // Projection type
        const char* projTypes[] = {"Perspective", "Orthographic"};
        int currentProj = static_cast<int>(camera.camera.projectionType());
        if (ImGui::Combo("Projection", &currentProj, projTypes, 2)) {
            if (currentProj == 0) {
                camera.camera.setPerspective(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
            } else {
                camera.camera.setOrthographicSize(10.0f, 16.0f/9.0f, 0.1f, 100.0f);
            }
        }
        
        // Perspective settings
        if (camera.camera.projectionType() == Rendering::ProjectionType::Perspective) {
            float fov = camera.camera.fieldOfView();
            if (ImGui::SliderFloat("Field of View", &fov, 1.0f, 179.0f)) {
                camera.camera.setPerspective(fov, camera.camera.aspectRatio(), 
                    camera.camera.nearPlane(), camera.camera.farPlane());
            }
        }
        
        // Near/Far planes
        float nearPlane = camera.camera.nearPlane();
        float farPlane = camera.camera.farPlane();
        bool changed = false;
        changed |= ImGui::DragFloat("Near Plane", &nearPlane, 0.01f, 0.01f, 100.0f);
        changed |= ImGui::DragFloat("Far Plane", &farPlane, 1.0f, 1.0f, 10000.0f);
        
        if (changed) {
            if (camera.camera.projectionType() == Rendering::ProjectionType::Perspective) {
                camera.camera.setPerspective(camera.camera.fieldOfView(), 
                    camera.camera.aspectRatio(), nearPlane, farPlane);
            } else {
                camera.camera.setOrthographicSize(10.0f, camera.camera.aspectRatio(), 
                    nearPlane, farPlane);
            }
        }
        
        // Culling mask (layer-based)
        ImGui::Text("Culling Mask: 0x%08X", camera.cullingMask);
    }
}
```

---

## Step 7: Testing

### Unit Test: `tests/test_camera.cpp`

```cpp
#include <catch2/catch_test_macros.hpp>
#include "Artifact/Rendering/Camera.h"

using namespace Artifact::Rendering;

TEST_CASE("Camera - Perspective Projection", "[Camera]") {
    Camera camera(ProjectionType::Perspective);
    camera.setPerspective(60.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    
    REQUIRE(camera.projectionType() == ProjectionType::Perspective);
    REQUIRE(camera.fieldOfView() == 60.0f);
    REQUIRE(camera.nearPlane() == 0.1f);
    REQUIRE(camera.farPlane() == 100.0f);
    
    // Projection matrix should be non-identity
    glm::mat4 proj = camera.projectionMatrix();
    REQUIRE(proj != glm::mat4(1.0f));
}

TEST_CASE("Camera - LookAt", "[Camera]") {
    Camera camera;
    camera.setPosition({0.0f, 0.0f, 10.0f});
    camera.lookAt({0.0f, 0.0f, 0.0f});
    
    glm::vec3 forward = camera.forward();
    REQUIRE(glm::length(forward) == Catch::Approx(1.0f));
    REQUIRE(forward.z < 0.0f); // Looking down -Z
}

TEST_CASE("Camera - Frustum Culling", "[Camera]") {
    Camera camera;
    camera.setPosition({0.0f, 0.0f, 0.0f});
    camera.lookAt({0.0f, 0.0f, -1.0f});
    camera.setPerspective(90.0f, 1.0f, 0.1f, 100.0f);
    
    const Frustum& frustum = camera.frustum();
    
    // Point in front of camera
    REQUIRE(frustum.contains({0.0f, 0.0f, -10.0f}));
    
    // Point behind camera
    REQUIRE_FALSE(frustum.contains({0.0f, 0.0f, 10.0f}));
}
```

---

## Step 8: Performance Validation

### Tracy Profiling

```cpp
// In Renderer::renderScene()
void Renderer::renderScene(entt::registry& registry) {
    ARTIFACT_ZONE_NAMED("Renderer::renderScene");
    
    {
        ARTIFACT_ZONE_NAMED("Get Primary Camera");
        Camera* primaryCam = CameraSystem::getPrimaryCamera(registry);
        if (!primaryCam) return;
    }
    
    {
        ARTIFACT_ZONE_NAMED("Frustum Culling");
        const Frustum& frustum = primaryCam->frustum();
        // ... culling code
    }
    
    // ... rest of rendering
}
```

**Target:** `Get Primary Camera` <0.01ms, `Frustum Culling` <0.1ms for 10k entities

---

## Step 9: Documentation

### User-Facing: `docs/LUA_API_REFERENCE.md`

```markdown
## Camera API (Lua)

### Camera.SetPosition(x, y, z)
Sets the camera position in world space.

**Example:**
```lua
function ExampleScript:start()
    Camera.SetPosition(0, 10, 20)
end
```

### Camera.LookAt(targetX, targetY, targetZ)
Points the camera at a target position.

**Example:**
```lua
Camera.LookAt(0, 0, 0) -- Look at world origin
```
```

---

## Migration Checklist

- [ ] Implement `Camera.h` and `Camera.cpp`
- [ ] Add `CameraComponent` to `Components.h`
- [ ] Implement `CameraSystem.h` and `CameraSystem.cpp`
- [ ] Update `Renderer` to use `CameraSystem::getPrimaryCamera()`
- [ ] Add Lua bindings for camera control
- [ ] Update `InspectorPanel` to edit camera properties
- [ ] Write unit tests (`test_camera.cpp`)
- [ ] Profile with Tracy (ensure <0.1ms overhead)
- [ ] Update CMakeLists.txt to include new files
- [ ] **Deprecate** `CameraController.lua` (move to `Assets/Raw/Scripts/Deprecated/`)
- [ ] Document new camera API in `LUA_API_REFERENCE.md`

---

## Success Criteria

✅ Camera transform updates run in <0.01ms  
✅ Frustum culling correctly rejects off-screen entities  
✅ Lua scripts can still control camera (backwards compatibility)  
✅ Inspector panel allows full camera configuration  
✅ Zero crashes or memory leaks (run under Valgrind/ASan)  

---

## Next Steps After Camera

1. **Mesh/Material System** (see `RENDERING_ARCHITECTURE.md`)
2. **Input System** (move keyboard queries to C++)
3. **Physics Components** (integrate Box2D/Jolt)


