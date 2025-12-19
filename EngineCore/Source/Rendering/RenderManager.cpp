#include "Artifact/Rendering/RenderManager.h"

#include <array>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "Artifact/Assets/MeshManager.h"
#include "Artifact/Assets/TextureLoader.h"
#include "Artifact/Core/Paths.h"
#include "Artifact/Core/LogManager.h"
#include "Artifact/Core/Macros.h"
#include "Artifact/Scene/Components.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "Artifact/Core/Instrumentation.h"

namespace {
    std::string LoadShaderSource(std::string_view relativePath) {
        const auto path = Artifact::Core::Paths::ResolveAssetPath(relativePath);
        std::ifstream file(path.string().c_str());
        if (!file.is_open()) {
            LOG_ERROR("Failed to open shader: {}", path.string());
            return {};
        }
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    GLuint CompileShader(GLenum type, const char *source);

    GLuint CreateProgram(const char *vertexSource, const char *fragmentSource);
} // namespace

namespace {
    GLuint CompileShader(GLenum type, const char *source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            LOG_ERROR("Failed to compile shader: {}", infoLog);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    GLuint CreateProgram(const char *vertexSource, const char *fragmentSource) {
        GLuint program = glCreateProgram();
        GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
        GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

        if (vertexShader == 0 || fragmentShader == 0) {
            return 0;
        }

        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return program;
    }
}

namespace Artifact::Rendering {
    namespace {
        constexpr const char *kDefaultSkyboxVert = "Shaders/skybox.vert";
        constexpr const char *kDefaultSkyboxFrag = "Shaders/skybox.frag";
        constexpr const char *kDefaultBasicVert = "Shaders/basic.vert";
        constexpr const char *kDefaultBasicFrag = "Shaders/basic.frag";
    }

    RenderManager::RenderManager() {
        LOG_INFO("RenderManager: Initializing");

        InitializeGlState();
        m_meshManager = std::make_unique<Assets::MeshManager>();
        m_basicShader = 0;

        const auto skyVert = LoadShaderSource(kDefaultSkyboxVert);
        const auto skyFrag = LoadShaderSource(kDefaultSkyboxFrag);
        m_skyboxProgram = CreateProgram(skyVert.c_str(), skyFrag.c_str());
        if (m_skyboxProgram == 0) {
            LOG_WARN("Skybox shader unavailable; background gradient disabled");
        } else {
            glGenVertexArrays(1, &m_skyboxVao);
            glBindVertexArray(m_skyboxVao);
            glBindVertexArray(0);
            m_skyboxTopColorLocation = glGetUniformLocation(m_skyboxProgram, "uSkyTopColor");
            m_skyboxHorizonColorLocation = glGetUniformLocation(m_skyboxProgram, "uSkyHorizonColor");
        }

        const auto basicVert = LoadShaderSource(kDefaultBasicVert);
        const auto basicFrag = LoadShaderSource(kDefaultBasicFrag);
        m_basicShader = CreateProgram(basicVert.c_str(), basicFrag.c_str());
        if (!m_basicShader) {
            LOG_ERROR("Failed to create basic shader program");
        }

        LOG_INFO("RenderManager: Initialization complete");
    }

    RenderManager::~RenderManager() {
        LOG_INFO("RenderManager: Shutting down");

        DestroySkybox();
        DestroySceneFramebuffer();
        DestroyPreviewFramebuffer();
        if (m_basicShader) {
            glDeleteProgram(m_basicShader);
            m_basicShader = 0;
        }
        m_meshManager.reset();
    }

    void RenderManager::BeginFrame(const FrameContext &ctx) {
        ARTIFACT_ZONE();
        glViewport(0, 0, ctx.windowWidth, ctx.windowHeight);
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RenderManager::EndFrame() {
        ARTIFACT_ZONE();
    }

    void RenderManager::BeginSceneViewport(int width, int height) {
        ARTIFACT_ZONE();
        EnsureSceneFramebuffer(width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFramebuffer);
        glViewport(0, 0, m_sceneWidth, m_sceneHeight);
        glClearColor(0.11f, 0.11f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RenderManager::EndSceneViewport() {
        ARTIFACT_ZONE();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    uint32_t RenderManager::SceneColorTexture() const {
        return m_sceneColor;
    }

    void RenderManager::DrawSkybox() {
        if (m_skyboxProgram == 0 || m_skyboxVao == 0) {
            return;
        }
        glDisable(GL_DEPTH_TEST);
        glUseProgram(m_skyboxProgram);
        if (m_skyboxTopColorLocation >= 0) {
            glUniform3fv(m_skyboxTopColorLocation, 1, m_skyboxTopColor.data());
        }
        if (m_skyboxHorizonColorLocation >= 0) {
            glUniform3fv(m_skyboxHorizonColorLocation, 1, m_skyboxHorizonColor.data());
        }
        glBindVertexArray(m_skyboxVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glUseProgram(0);
        glEnable(GL_DEPTH_TEST);
    }

    void RenderManager::SetSkyboxColors(const std::array<float, 3> &top, const std::array<float, 3> &horizon) {
        m_skyboxTopColor = top;
        m_skyboxHorizonColor = horizon;
    }

    void RenderManager::RenderScene(const Scene::SceneContext &scene, Rendering::Camera *overrideCamera) {
        ARTIFACT_ZONE();

        Rendering::Camera *camera = ResolveCamera(scene, overrideCamera);
        DrawSkybox();

        if (!camera || m_basicShader == 0) {
            return;
        }

        DrawSceneMeshes(scene, camera->viewProjectionMatrix());
    }

    void RenderManager::DrawSceneMeshes(const Scene::SceneContext &scene, const glm::mat4 &viewProjection) {
        if (m_basicShader == 0) {
            return;
        }
        
        // Ensure culling is enabled at the start of mesh rendering
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        
        glUseProgram(m_basicShader);
        GLint modelLoc = glGetUniformLocation(m_basicShader, "uModel");
        GLint vpLoc = glGetUniformLocation(m_basicShader, "uViewProj");
        GLint colorLoc = glGetUniformLocation(m_basicShader, "uColor");
        glUniformMatrix4fv(vpLoc, 1, GL_FALSE, &viewProjection[0][0]);

        if (!m_meshManager) {
            m_meshManager = std::make_unique<Assets::MeshManager>();
        }

        auto view = scene.registry.view<Scene::TransformComponent, Scene::MeshComponent, Scene::MaterialComponent>();
        for (auto entity: view) {
            auto &transform = view.get<Scene::TransformComponent>(entity);
            auto &meshComp = view.get<Scene::MeshComponent>(entity);
            auto &material = view.get<Scene::MaterialComponent>(entity);

            const auto *mesh = m_meshManager->loadMesh(meshComp.meshPath);
            if (!mesh || mesh->vertices.empty()) {
                continue;
            }

            MeshDrawData drawData = UploadMesh(*mesh);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, transform.transform.position);
            // Apply rotation using proper XYZ Euler angles (X=pitch, Y=yaw, Z=roll)
            model = model * glm::eulerAngleXYZ(glm::radians(transform.transform.rotation.x),
                                               glm::radians(transform.transform.rotation.y),
                                               glm::radians(transform.transform.rotation.z));
            model = glm::scale(model, transform.transform.scale);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
            glUniform4fv(colorLoc, 1, &material.color[0]);

            // Handle double-sided rendering
            if (material.doubleSided) {
                glDisable(GL_CULL_FACE);
            }

            // Bind Texture
            if (!material.albedoTexture.empty()) {
                // Simple texture cache/lookup could be added here, but for now we assume AssetManager handles it or we load it
                // For this assignment, let's assume we need to load it or get it from a TextureManager.
                // Since we don't have a global TextureManager accessible here easily without including it,
                // let's use a static map or similar for this simple renderer, or better, use the AssetManager if possible.
                // Wait, TextureLoader is in Assets.
                // Let's just load it for now (inefficient but works for "Cube is textured")
                // Ideally we should cache this GLuint.
                // For the sake of the assignment "Cube is textured", we need to bind A texture.
                // Let's check if we have a texture loaded.
            }

            // TODO: Proper texture management. For now, we rely on the user assigning a texture.
            // We need a way to get the GLuint from the path.
            // Let's add a simple texture cache to RenderManager for this assignment.
            GLuint textureId = GetTexture(material.albedoTexture);
            GLint hasTexLoc = glGetUniformLocation(m_basicShader, "uHasTexture");

            if (textureId != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureId);
                // Assuming shader has "uAlbedoMap" or similar, or just defaults to unit 0
                // We need to set the uniform if it exists.
                GLint texLoc = glGetUniformLocation(m_basicShader, "uAlbedoMap");
                if (texLoc != -1) glUniform1i(texLoc, 0);

                if (hasTexLoc != -1) glUniform1i(hasTexLoc, 1);
            } else {
                if (hasTexLoc != -1) glUniform1i(hasTexLoc, 0);
            }

            glBindVertexArray(drawData.vao);
            glDrawElements(GL_TRIANGLES, drawData.indexCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);

            // Restore face culling if it was disabled for double-sided rendering
            if (material.doubleSided) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            }

            DestroyMesh(drawData);
        }

        glUseProgram(0);
    }

    uint32_t RenderManager::RenderScenePreview(const Scene::SceneContext &scene, int width, int height,
                                               Rendering::Camera *overrideCamera) {
        EnsurePreviewFramebuffer(width, height);
        if (m_previewFramebuffer == 0) {
            return 0;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_previewFramebuffer);
        glViewport(0, 0, width, height);
        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Rendering::Camera *camera = ResolveCamera(scene, overrideCamera);
        DrawSkybox();
        if (camera && m_basicShader) {
            DrawSceneMeshes(scene, camera->viewProjectionMatrix());
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return m_previewColor;
    }

    Rendering::Camera *
    RenderManager::ResolveCamera(const Scene::SceneContext &scene, Rendering::Camera *overrideCamera) {
        if (overrideCamera) {
            return overrideCamera;
        }

        Rendering::Camera *runtimeCamera = m_cameraSystem.
                getPrimaryCamera(const_cast<entt::registry &>(scene.registry));
        if (runtimeCamera) {
            return runtimeCamera;
        }

        return PrepareFallbackCamera();
    }

    Rendering::Camera *RenderManager::PrepareFallbackCamera() {
        constexpr glm::vec3 fallbackPosition{0.0f, 2.5f, 5.0f};
        constexpr glm::vec3 fallbackTarget{0.0f, 0.0f, 0.0f};
        m_fallbackCamera.setPosition(fallbackPosition);
        m_fallbackCamera.lookAt(fallbackTarget, glm::vec3(0.0f, 1.0f, 0.0f));

        const float aspect = m_sceneHeight > 0
                                 ? static_cast<float>(m_sceneWidth) / static_cast<float>(m_sceneHeight)
                                 : 16.0f / 9.0f;
        m_fallbackCamera.setPerspective(m_fallbackCamera.fieldOfView(), aspect, m_fallbackCamera.nearPlane(),
                                        m_fallbackCamera.farPlane());
        return &m_fallbackCamera;
    }

    MeshDrawData RenderManager::UploadMesh(const Assets::Mesh &mesh) {
        MeshDrawData data{};
        glGenVertexArrays(1, &data.vao);
        glGenBuffers(1, &data.vertexBuffer);
        glGenBuffers(1, &data.indexBuffer);

        glBindVertexArray(data.vao);
        glBindBuffer(GL_ARRAY_BUFFER, data.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Assets::MeshVertex), mesh.vertices.data(),
                     GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(),
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::MeshVertex),
                              reinterpret_cast<void *>(offsetof(Assets::MeshVertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::MeshVertex),
                              reinterpret_cast<void *>(offsetof(Assets::MeshVertex, normal)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Assets::MeshVertex),
                              reinterpret_cast<void *>(offsetof(Assets::MeshVertex, texcoord)));

        glBindVertexArray(0);

        data.indexCount = static_cast<GLsizei>(mesh.indices.size());
        return data;
    }

    void RenderManager::DestroyMesh(MeshDrawData &data) {
        if (data.indexBuffer) {
            glDeleteBuffers(1, &data.indexBuffer);
            data.indexBuffer = 0;
        }
        if (data.vertexBuffer) {
            glDeleteBuffers(1, &data.vertexBuffer);
            data.vertexBuffer = 0;
        }
        if (data.vao) {
            glDeleteVertexArrays(1, &data.vao);
            data.vao = 0;
        }
        data.indexCount = 0;
    }

    GLuint RenderManager::GetTexture(const std::string& path) {
        if (path.empty()) return 0;
        if (auto it = m_textureCache.find(path); it != m_textureCache.end()) {
            return it->second;
        }

        LOG_INFO("Loading texture: {}", path);
        auto data = Assets::TextureLoader::loadFromFile(path);
        if (!data.pixels) {
            LOG_ERROR("Failed to load texture: {}", path);
            return 0;
        }

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        GLenum format = GL_RGB;
        if (data.channels == 1) format = GL_RED;
        else if (data.channels == 3) format = GL_RGB;
        else if (data.channels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, data.width, data.height, 0, format, GL_UNSIGNED_BYTE, data.pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        m_textureCache[path] = textureID;
        LOG_INFO("Texture loaded successfully: {} (ID: {})", path, textureID);
        return textureID;
    }

    void RenderManager::InitializeGlState() {
        ARTIFACT_ZONE();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    void RenderManager::EnsureSceneFramebuffer(int width, int height) {
        if (width <= 0 || height <= 0) {
            return;
        }
        if (width == m_sceneWidth && height == m_sceneHeight && m_sceneFramebuffer != 0) {
            return;
        }
        DestroySceneFramebuffer();

        glGenFramebuffers(1, &m_sceneFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFramebuffer);

        glGenTextures(1, &m_sceneColor);
        glBindTexture(GL_TEXTURE_2D, m_sceneColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_sceneColor, 0);

        glGenTextures(1, &m_sceneDepth);
        glBindTexture(GL_TEXTURE_2D, m_sceneDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                     nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_sceneDepth, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR("Scene framebuffer incomplete");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_sceneWidth = width;
        m_sceneHeight = height;
    }

    void RenderManager::DestroySceneFramebuffer() {
        if (m_sceneDepth != 0) {
            glDeleteTextures(1, &m_sceneDepth);
            m_sceneDepth = 0;
        }
        if (m_sceneColor != 0) {
            glDeleteTextures(1, &m_sceneColor);
            m_sceneColor = 0;
        }
        if (m_sceneFramebuffer != 0) {
            glDeleteFramebuffers(1, &m_sceneFramebuffer);
            m_sceneFramebuffer = 0;
        }
        m_sceneWidth = 0;
        m_sceneHeight = 0;
    }

    void RenderManager::DestroySkybox() {
        if (m_skyboxVao != 0) {
            glDeleteVertexArrays(1, &m_skyboxVao);
            m_skyboxVao = 0;
        }
        if (m_skyboxProgram != 0) {
            glDeleteProgram(m_skyboxProgram);
            m_skyboxProgram = 0;
        }
        m_skyboxTopColorLocation = -1;
        m_skyboxHorizonColorLocation = -1;
    }

    Rendering::Camera *RenderManager::BindRuntimeCamera(entt::registry &registry) {
        return m_cameraSystem.getPrimaryCamera(registry);
    }

    void RenderManager::EnsurePreviewFramebuffer(int width, int height) {
        if (width <= 0 || height <= 0) {
            return;
        }
        if (width == m_previewWidth && height == m_previewHeight && m_previewFramebuffer != 0) {
            return;
        }

        DestroyPreviewFramebuffer();

        glGenFramebuffers(1, &m_previewFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_previewFramebuffer);

        glGenTextures(1, &m_previewColor);
        glBindTexture(GL_TEXTURE_2D, m_previewColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_previewColor, 0);

        glGenTextures(1, &m_previewDepth);
        glBindTexture(GL_TEXTURE_2D, m_previewDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                     nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_previewDepth, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR("Preview framebuffer incomplete");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_previewWidth = width;
        m_previewHeight = height;
    }

    void RenderManager::DestroyPreviewFramebuffer() {
        if (m_previewDepth != 0) {
            glDeleteTextures(1, &m_previewDepth);
            m_previewDepth = 0;
        }
        if (m_previewColor != 0) {
            glDeleteTextures(1, &m_previewColor);
            m_previewColor = 0;
        }
        if (m_previewFramebuffer != 0) {
            glDeleteFramebuffers(1, &m_previewFramebuffer);
            m_previewFramebuffer = 0;
        }
        m_previewWidth = 0;
        m_previewHeight = 0;
    }
} // namespace Artifact::Rendering
