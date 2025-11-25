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

    const Frustum &Camera::frustum() const {
        if (m_frustumDirty) {
            updateFrustum();
            m_frustumDirty = false;
        }
        return m_frustum;
    }

    const glm::mat4 &Camera::projectionMatrix() const {
        if (m_projectionDirty) {
            updateProjectionMatrix();
            m_projectionDirty = false;
        }
        return m_projectionMatrix;
    }

    const glm::mat4 &Camera::viewMatrix() const {
        if (m_viewDirty) {
            updateViewMatrix();
            m_viewDirty = false;
        }
        return m_viewMatrix;
    }

    glm::mat4 Camera::viewProjectionMatrix() const {
        return projectionMatrix() * viewMatrix();
    }

    glm::vec3 Camera::up() const {
        return glm::normalize(m_rotation * glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 Camera::right() const {
        return glm::normalize(m_rotation * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 Camera::forward() const {
        return glm::normalize(m_rotation * glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 Camera::eulerAngles() const {
        return glm::degrees(glm::eulerAngles(m_rotation));
    }

    glm::vec2 Camera::worldToScreenPoint(const glm::vec3 &worldPos, const glm::vec2 &viewportSize) const {
        glm::vec4 clip = viewProjectionMatrix() * glm::vec4(worldPos, 1.0f);
        if (clip.w == 0.0f) {
            return glm::vec2(std::numeric_limits<float>::infinity());
        }
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        glm::vec2 screen;
        screen.x = (ndc.x + 1.0f) * 0.5f * viewportSize.x;
        // NDC y up -> screen y down
        screen.y = (1.0f - ndc.y) * 0.5f * viewportSize.y;
        return screen;
    }

    glm::vec3 Camera::screenToWorldPoint(const glm::vec2 &screenPos, float depth, const glm::vec2 &viewportSize) const {
        // Convert screen to NDC
        glm::vec2 ndc;
        ndc.x = (2.0f * screenPos.x) / viewportSize.x - 1.0f;
        ndc.y = 1.0f - (2.0f * screenPos.y) / viewportSize.y; // flip

        glm::vec4 clip(ndc, depth, 1.0f);
        glm::mat4 invVP = glm::inverse(viewProjectionMatrix());
        glm::vec4 world = invVP * clip;
        if (world.w != 0.0f) {
            world /= world.w;
        }
        return glm::vec3(world);
    }

    Ray Camera::screenPointToRay(const glm::vec2 &screenPos, const glm::vec2 &viewportSize) const {
        glm::vec2 ndc = (screenPos / viewportSize) * 2.0f - 1.0f;
        ndc.y = -ndc.y;

        glm::vec4 rayClip(ndc.x, ndc.y, -1.0f, 1.0f);
        glm::vec4 rayEye = glm::inverse(projectionMatrix()) * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

        glm::vec3 rayWorld = glm::vec3(glm::inverse(viewMatrix()) * rayEye);
        rayWorld = glm::normalize(rayWorld);

        return Ray{m_position, rayWorld};
    }

    void Camera::setOrthographicSize(float size, float aspectRatio, float nearPlane, float farPlane) {
        float halfHeight = size;
        float halfWidth = halfHeight * aspectRatio;
        setOrthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
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

    void Camera::setPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane) {
        m_projectionType = ProjectionType::Perspective;
        m_fov = fovDegrees;
        m_aspectRatio = aspectRatio;
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
        m_projectionDirty = true;
        m_frustumDirty = true;
    }

    void Camera::lookAt(const glm::vec3 &target, const glm::vec3 &up) {
        glm::vec3 direction = glm::normalize(target - m_position);
        glm::vec3 right = glm::normalize(glm::cross(direction, up));
        glm::vec3 actualUp = glm::cross(right, direction);

        glm::mat3 rotationMatrix;
        rotationMatrix[0] = right;
        rotationMatrix[1] = actualUp;
        rotationMatrix[2] = -direction;

        m_rotation = glm::quat_cast(rotationMatrix);
        m_viewDirty = true;
        m_frustumDirty = true;
    }

    void Camera::setRotation(const glm::vec3 &eulerAngles) {
        glm::vec3 radians = glm::radians(eulerAngles);
        m_rotation = glm::quat(radians);
        m_viewDirty = true;
        m_frustumDirty = true;
    }

    void Camera::setRotation(const glm::quat &rotation) {
        m_rotation = glm::normalize(rotation);
        m_viewDirty = true;
        m_frustumDirty = true;
    }

    void Camera::setPosition(const glm::vec3 &position) {
        m_position = position;
        m_viewDirty = true;
        m_frustumDirty = true;
    }

    void Camera::updateFrustum() const {
        glm::mat4 vp = viewProjectionMatrix();

        m_frustum.planes[0] = glm::vec4(
            vp[0][3] + vp[0][0],
            vp[1][3] + vp[1][0],
            vp[2][3] + vp[2][0],
            vp[3][3] + vp[3][0]
        );

        m_frustum.planes[1] = glm::vec4(
            vp[0][3] - vp[0][0],
            vp[1][3] - vp[1][0],
            vp[2][3] - vp[2][0],
            vp[3][3] - vp[3][0]
        );

        m_frustum.planes[2] = glm::vec4(
            vp[0][3] + vp[0][1],
            vp[1][3] + vp[1][1],
            vp[2][3] + vp[2][1],
            vp[3][3] + vp[3][1]
        );

        m_frustum.planes[3] = glm::vec4(
            vp[0][3] - vp[0][1],
            vp[1][3] - vp[1][1],
            vp[2][3] - vp[2][1],
            vp[3][3] - vp[3][1]
        );

        m_frustum.planes[4] = glm::vec4(
            vp[0][3] + vp[0][2],
            vp[1][3] + vp[1][2],
            vp[2][3] + vp[2][2],
            vp[3][3] + vp[3][2]
        );

        m_frustum.planes[5] = glm::vec4(
            vp[0][3] - vp[0][2],
            vp[1][3] - vp[1][2],
            vp[2][3] - vp[2][2],
            vp[3][3] - vp[3][2]
        );

        for (int i = 0; i < 6; ++i) {
            float length = glm::length(glm::vec3(m_frustum.planes[i]));
            if (length > 0.0f) {
                m_frustum.planes[i] /= length;
            }
        }
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

    void Camera::updateViewMatrix() const {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), -m_position);
        glm::mat4 rotation = glm::mat4_cast(glm::conjugate(m_rotation));
        m_viewMatrix = rotation * translation;
    }

    bool Frustum::contains(const glm::vec3 &point) const {
        for (int i = 0; i < 6; ++i) {
            float distance = glm::dot(glm::vec3(planes[i]), point) + planes[i].w;
            if (distance < 0.0f) {
                return false;
            }
        }
        return true;
    }

    bool Frustum::intersects(const glm::vec3 &center, float radius) const {
        for (int i = 0; i < 6; ++i) {
            float distance = glm::dot(glm::vec3(planes[i]), center) + planes[i].w;
            if (distance < -radius) {
                return false;
            }
        }
        return true;
    }
} // namespace Artifact::Rendering
