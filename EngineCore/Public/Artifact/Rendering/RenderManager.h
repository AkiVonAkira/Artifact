#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <string>

#include <glad/glad.h>
#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>

#include "Artifact/Core/Singleton.h"
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
    class MeshManager;
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

    /**
     * @brief Central rendering system managing graphics state, framebuffers, and scene rendering
     *
     * RenderManager is a singleton that handles all rendering operations including:
     * - OpenGL state management
     * - Framebuffer management (scene and preview)
     * - Skybox rendering
     * - Scene mesh rendering
     * - Camera resolution and fallback
     *
     * Access via RenderManager::Instance()
     */
    class RenderManager : public Core::Singleton<RenderManager> {
    public:
        RenderManager();

        ~RenderManager() override;

        // Frame lifecycle
        void BeginFrame(const FrameContext &ctx);

        void EndFrame();

        // Scene viewport rendering
        void BeginSceneViewport(int width, int height);

        void EndSceneViewport();

        uint32_t SceneColorTexture() const;

        // Skybox
        void DrawSkybox();

        void SetSkyboxColors(const std::array<float, 3> &top, const std::array<float, 3> &horizon);

        // Scene rendering
        void RenderScene(const Scene::SceneContext &scene, Rendering::Camera *overrideCamera = nullptr);

        uint32_t RenderScenePreview(const Scene::SceneContext &scene, int width, int height,
                                    Rendering::Camera *overrideCamera = nullptr);

        Rendering::Camera *BindRuntimeCamera(entt::registry &registry);

    private:
        GLuint GetTexture(const std::string& path);

        void InitializeGlState();

        void DestroySkybox();

        void EnsureSceneFramebuffer(int width, int height);

        void DestroySceneFramebuffer();

        void EnsurePreviewFramebuffer(int width, int height);

        void DestroyPreviewFramebuffer();

        MeshDrawData UploadMesh(const Assets::Mesh &mesh);

        void DestroyMesh(MeshDrawData &data);

        void DrawSceneMeshes(const Scene::SceneContext &scene, const glm::mat4 &viewProjection);

        Rendering::Camera *ResolveCamera(const Scene::SceneContext &scene, Rendering::Camera *overrideCamera);

        Rendering::Camera *PrepareFallbackCamera();

        // Framebuffers
        GLuint m_sceneFramebuffer{0};
        GLuint m_sceneColor{0};
        GLuint m_sceneDepth{0};
        int m_sceneWidth{0};
        int m_sceneHeight{0};

        GLuint m_previewFramebuffer{0};
        GLuint m_previewColor{0};
        GLuint m_previewDepth{0};
        int m_previewWidth{0};
        int m_previewHeight{0};

        // Skybox
        GLuint m_skyboxProgram{0};
        GLuint m_skyboxVao{0};
        GLint m_skyboxTopColorLocation{-1};
        GLint m_skyboxHorizonColorLocation{-1};
        std::array<float, 3> m_skyboxTopColor{0.09f, 0.12f, 0.22f};
        std::array<float, 3> m_skyboxHorizonColor{0.25f, 0.32f, 0.45f};

        // Rendering
        GLuint m_basicShader{0};
        Rendering::CameraSystem m_cameraSystem{};
        Rendering::Camera m_fallbackCamera{Rendering::ProjectionType::Perspective};
        std::unique_ptr<Assets::MeshManager> m_meshManager;
        std::unordered_map<std::string, GLuint> m_textureCache;
    };
} // namespace Artifact::Rendering
