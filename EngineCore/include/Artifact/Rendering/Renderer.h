#pragma once

#include <array>
#include <cstdint>

#include "Artifact/Rendering/Camera.h"
#include "Artifact/Rendering/CameraSystem.h"
#include "Artifact/Scene/SceneContext.h"

namespace Artifact::Rendering {

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

    unsigned int m_sceneFramebuffer{0};
    unsigned int m_sceneColor{0};
    unsigned int m_sceneDepth{0};
    int m_sceneWidth{0};
    int m_sceneHeight{0};

    unsigned int m_skyboxProgram{0};
    unsigned int m_skyboxVao{0};
    int m_skyboxTopColorLocation{-1};
    int m_skyboxHorizonColorLocation{-1};
    std::array<float, 3> m_skyboxTopColor{0.09f, 0.12f, 0.22f};
    std::array<float, 3> m_skyboxHorizonColor{0.25f, 0.32f, 0.45f};

    CameraSystem m_cameraSystem{};
};

}
