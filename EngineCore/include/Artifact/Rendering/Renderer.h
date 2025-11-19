#pragma once

#include <array>
#include <cstdint>

#include <glad/glad.h>
#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>

#include "Artifact/Rendering/Camera.h"
#include "Artifact/Rendering/CameraSystem.h"
#include "Artifact/Scene/SceneContext.h"
#include "Artifact/Assets/Mesh.h"
#include "Artifact/Core/Paths.h"

namespace Artifact::Scene {
struct SceneContext;
}

namespace Artifact::Assets {
class Mesh;
}

namespace Artifact::Rendering {

struct MeshDrawData {
    GLuint vao{0};
    GLuint vertexBuffer{0};
    GLuint indexBuffer{0};
    GLsizei indexCount{0};
};

struct FrameContext {
    float deltaSeconds{0.0f};
    int windowWidth{0};
    int windowHeight{0};
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    void beginFrame(const FrameContext& ctx);
    void endFrame();

    void beginSceneViewport(int width, int height);
    void endSceneViewport();
    uint32_t sceneColorTexture() const;
    void drawSkybox();

    void renderScene(const Scene::SceneContext& scene, Rendering::Camera* overrideCamera = nullptr);
    Rendering::Camera* bindRuntimeCamera(entt::registry& registry);

private:
    void initializeGlState();
    void initializeSkybox();
    void destroySkybox();
    void ensureSceneFramebuffer(int width, int height);
    void destroySceneFramebuffer();

    MeshDrawData uploadMesh(const Assets::Mesh& mesh);
    void destroyMesh(MeshDrawData& data);

    GLuint m_sceneFramebuffer{0};
    GLuint m_sceneColor{0};
    GLuint m_sceneDepth{0};
    int m_sceneWidth{0};
    int m_sceneHeight{0};

    GLuint m_skyboxProgram{0};
    GLuint m_skyboxVao{0};
    GLint m_skyboxTopColorLocation{-1};
    GLint m_skyboxHorizonColorLocation{-1};
    std::array<float, 3> m_skyboxTopColor{0.09f, 0.12f, 0.22f};
    std::array<float, 3> m_skyboxHorizonColor{0.25f, 0.32f, 0.45f};

    Rendering::CameraSystem m_cameraSystem{};
    GLuint m_basicShader{0};
};

} // namespace Artifact::Rendering
