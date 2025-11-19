#include "Artifact/Rendering/Renderer.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "Artifact/Assets/MeshManager.h"
#include "Artifact/Core/Paths.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>

#include "Artifact/Core/Instrumentation.h"

namespace {
std::string LoadShaderSource(std::string_view relativePath) {
    const auto path = Artifact::Core::Paths::ResolveAssetPath(relativePath);
    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::error("Failed to open shader: {}", path.string());
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint CreateProgram(const char* vertexSource, const char* fragmentSource);

} // namespace

namespace {
GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        spdlog::error("Failed to compile shader: {}", infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint CreateProgram(const char* vertexSource, const char* fragmentSource) {
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
        constexpr const char* kDefaultSkyboxVert = "Assets/Shaders/skybox.vert";
        constexpr const char* kDefaultSkyboxFrag = "Assets/Shaders/skybox.frag";
        constexpr const char* kDefaultBasicVert = "Assets/Shaders/basic.vert";
        constexpr const char* kDefaultBasicFrag = "Assets/Shaders/basic.frag";
    }

    Renderer::Renderer() {
        initializeGlState();
        const auto skyVert = LoadShaderSource(kDefaultSkyboxVert);
        const auto skyFrag = LoadShaderSource(kDefaultSkyboxFrag);
        m_skyboxProgram = CreateProgram(skyVert.c_str(), skyFrag.c_str());
        if (m_skyboxProgram == 0) {
            spdlog::warn("Skybox shader unavailable; background gradient disabled");
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
            spdlog::error("Failed to create basic shader program");
        }
    }

    Renderer::~Renderer() {
        destroySkybox();
        destroySceneFramebuffer();
        if (m_basicShader) {
            glDeleteProgram(m_basicShader);
            m_basicShader = 0;
        }
    }

    void Renderer::beginFrame(const FrameContext& ctx) {
        ARTIFACT_ZONE();
        glViewport(0, 0, ctx.windowWidth, ctx.windowHeight);
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::endFrame() {
        ARTIFACT_ZONE();
    }

    void Renderer::beginSceneViewport(int width, int height) {
        ARTIFACT_ZONE();
        ensureSceneFramebuffer(width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFramebuffer);
        glViewport(0, 0, m_sceneWidth, m_sceneHeight);
        glClearColor(0.11f, 0.11f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::endSceneViewport() {
        ARTIFACT_ZONE();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    uint32_t Renderer::sceneColorTexture() const {
        return m_sceneColor;
    }

    void Renderer::drawSkybox() {
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

    void Renderer::renderScene(const Scene::SceneContext& scene, Rendering::Camera* overrideCamera) {
        ARTIFACT_ZONE();

        Rendering::Camera* camera = overrideCamera;
        if (!camera) {
            camera = m_cameraSystem.getPrimaryCamera(const_cast<entt::registry&>(scene.registry));
        }
        if (!camera) {
            drawSkybox();
            return;
        }

        auto viewProjection = camera->viewProjectionMatrix();

        glUseProgram(m_basicShader);
        GLint modelLoc = glGetUniformLocation(m_basicShader, "uModel");
        GLint vpLoc = glGetUniformLocation(m_basicShader, "uViewProj");
        GLint colorLoc = glGetUniformLocation(m_basicShader, "uColor");
        glUniformMatrix4fv(vpLoc, 1, GL_FALSE, &viewProjection[0][0]);

        Assets::MeshManager meshManager; // temporary stack manager
        auto view = scene.registry.view<Scene::TransformComponent, Scene::MeshComponent, Scene::MaterialComponent>();
        for (auto entity : view) {
            auto& transform = view.get<Scene::TransformComponent>(entity);
            auto& meshComp = view.get<Scene::MeshComponent>(entity);
            auto& material = view.get<Scene::MaterialComponent>(entity);

            const auto* mesh = meshManager.loadMesh(meshComp.meshPath);
            if (!mesh || mesh->vertices.empty()) {
                continue;
            }

            MeshDrawData drawData = uploadMesh(*mesh);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, transform.transform.position);
            model = model * glm::yawPitchRoll(glm::radians(transform.transform.rotation.y),
                                              glm::radians(transform.transform.rotation.x),
                                              glm::radians(transform.transform.rotation.z));
            model = glm::scale(model, transform.transform.scale);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
            glUniform4fv(colorLoc, 1, &material.color[0]);

            glBindVertexArray(drawData.vao);
            glDrawElements(GL_TRIANGLES, drawData.indexCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);

            destroyMesh(drawData);
        }

        glUseProgram(0);
    }

    MeshDrawData Renderer::uploadMesh(const Assets::Mesh& mesh) {
        MeshDrawData data{};
        glGenVertexArrays(1, &data.vao);
        glGenBuffers(1, &data.vertexBuffer);
        glGenBuffers(1, &data.indexBuffer);

        glBindVertexArray(data.vao);
        glBindBuffer(GL_ARRAY_BUFFER, data.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Assets::MeshVertex), mesh.vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::MeshVertex), reinterpret_cast<void*>(offsetof(Assets::MeshVertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::MeshVertex), reinterpret_cast<void*>(offsetof(Assets::MeshVertex, normal)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Assets::MeshVertex), reinterpret_cast<void*>(offsetof(Assets::MeshVertex, texcoord)));

        glBindVertexArray(0);

        data.indexCount = static_cast<GLsizei>(mesh.indices.size());
        return data;
    }

    void Renderer::destroyMesh(MeshDrawData& data) {
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

    void Renderer::initializeGlState() {
        ARTIFACT_ZONE();
        glEnable(GL_DEPTH_TEST);
    }

    void Renderer::ensureSceneFramebuffer(int width, int height) {
        if (width <= 0 || height <= 0) {
            return;
        }
        if (width == m_sceneWidth && height == m_sceneHeight && m_sceneFramebuffer != 0) {
            return;
        }
        destroySceneFramebuffer();

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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_sceneDepth, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            spdlog::error("Scene framebuffer incomplete");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_sceneWidth = width;
        m_sceneHeight = height;
    }

    void Renderer::destroySceneFramebuffer() {
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

    void Renderer::destroySkybox() {
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

    Rendering::Camera* Renderer::bindRuntimeCamera(entt::registry& registry) {
        return m_cameraSystem.getPrimaryCamera(registry);
    }
}
