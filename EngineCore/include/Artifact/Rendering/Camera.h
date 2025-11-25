#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Artifact::Rendering {
    enum class ProjectionType {
        Orthographic,
        Perspective
    };

    struct Ray {
        glm::vec3 origin{0.0f};
        glm::vec3 direction{0.0f};

        Ray() = default;

        Ray(const glm::vec3 &o, const glm::vec3 &d) : origin(o), direction(glm::normalize(d)) {
        }
    };

    struct Frustum {
        // 6 planes (a, b, c, d) stored as glm::vec4: ax + by + cz + d = 0
        std::array<glm::vec4, 6> planes{};

        Frustum() = default;

        // Frustum queries
        bool contains(const glm::vec3 &point) const;

        bool intersects(const glm::vec3 &center, float radius) const;
    };

    class Camera {
    public:
        explicit Camera(ProjectionType type = ProjectionType::Perspective);

        // --- Basic properties / getters ---
        float farPlane() const { return m_farPlane; }
        float nearPlane() const { return m_nearPlane; }
        float aspectRatio() const { return m_aspectRatio; }
        float fieldOfView() const { return m_fov; } // degrees
        ProjectionType projectionType() const { return m_projectionType; }

        const Frustum &frustum() const;

        const glm::mat4 &projectionMatrix() const;

        const glm::mat4 &viewMatrix() const;

        glm::mat4 viewProjectionMatrix() const;

        const glm::quat &rotation() const { return m_rotation; }
        const glm::vec3 &position() const { return m_position; }

        glm::vec3 up() const;

        glm::vec3 right() const;

        glm::vec3 forward() const;

        glm::vec3 eulerAngles() const;

        // --- Coordinate conversions ---
        // world -> screen (in pixels). viewportSize is (width, height).
        glm::vec2 worldToScreenPoint(const glm::vec3 &worldPos, const glm::vec2 &viewportSize) const;

        // screen -> world point at given depth (in view space or world? choose world-space depth along view ray)
        glm::vec3 screenToWorldPoint(const glm::vec2 &screenPos, float depth, const glm::vec2 &viewportSize) const;

        // produce a ray from screen point into the world
        Ray screenPointToRay(const glm::vec2 &screenPos, const glm::vec2 &viewportSize) const;

        // --- Projection / view setters ---
        void setOrthographicSize(float size, float aspectRatio, float nearPlane, float farPlane);

        void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);

        void setPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane);

        void lookAt(const glm::vec3 &target, const glm::vec3 &up = glm::vec3(0.0f, 1.0f, 0.0f));

        void setRotation(const glm::vec3 &eulerAngles); // degrees
        void setRotation(const glm::quat &rotation);

        void setPosition(const glm::vec3 &position);

        // --- Frustum queries convenience ---
        bool intersects(const glm::vec3 &center, float radius) const { return frustum().intersects(center, radius); }
        bool contains(const glm::vec3 &point) const { return frustum().contains(point); }

    private:
        mutable bool m_frustumDirty{true};
        mutable bool m_projectionDirty{true};
        mutable bool m_viewDirty{true};

        mutable Frustum m_frustum{};
        mutable glm::mat4 m_projectionMatrix{1.0f};
        mutable glm::mat4 m_viewMatrix{1.0f};

        // orthographic extents
        float m_orthoTop{10.0f};
        float m_orthoBottom{-10.0f};
        float m_orthoRight{10.0f};
        float m_orthoLeft{-10.0f};

        // perspective parameters
        float m_farPlane{1000.0f};
        float m_nearPlane{0.1f};
        float m_aspectRatio{16.0f / 9.0f};
        float m_fov{45.0f}; // degrees
        ProjectionType m_projectionType{ProjectionType::Perspective};

        glm::quat m_rotation{1.0f, 0.0f, 0.0f, 0.0f}; // w, x, y, z
        glm::vec3 m_position{0.0f, 0.0f, 0.0f};

        // update helpers (mutable because getters are const)
        void updateFrustum() const;

        void updateProjectionMatrix() const;

        void updateViewMatrix() const;
    };
} // namespace Artifact::Rendering
