#include "Artifact/Rendering/Renderer.h"

#include <glad/glad.h>

#include "Artifact/Core/Instrumentation.h"
#include <spdlog/spdlog.h>
#include <string>

namespace {

constexpr const char* kSkyboxVertexShader = R"GLSL(
#version 330 core
const vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2(3.0, -1.0),
    vec2(-1.0, 3.0)
);
out vec2 vUV;
void main() {
    vec2 clipPos = positions[gl_VertexID];
    vUV = clipPos * 0.5 + 0.5;
    gl_Position = vec4(clipPos, 0.0, 1.0);
}
)GLSL";

constexpr const char* kSkyboxFragmentShader = R"GLSL(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform vec3 uSkyTopColor;
uniform vec3 uSkyHorizonColor;
void main() {
    float t = clamp(vUV.y, 0.0, 1.0);
    vec3 color = mix(uSkyHorizonColor, uSkyTopColor, t);
    FragColor = vec4(color, 1.0);
}
)GLSL";

GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<size_t>(logLength), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        spdlog::error("Skybox shader compilation failed: {}", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint CreateProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (vertexShader == 0 || fragmentShader == 0) {
        if (vertexShader != 0) {
            glDeleteShader(vertexShader);
        }
        if (fragmentShader != 0) {
            glDeleteShader(fragmentShader);
        }
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<size_t>(logLength), '\0');
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        spdlog::error("Skybox program link failed: {}", log);
        glDeleteProgram(program);
        program = 0;
    }

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

}

namespace Artifact::Rendering {

Renderer::Renderer() {
    initializeGlState();
    initializeSkybox();
}

Renderer::~Renderer() {
    destroySkybox();
    destroySceneFramebuffer();
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

void Renderer::initializeGlState() {
    ARTIFACT_ZONE();
    glEnable(GL_DEPTH_TEST);
}

void Renderer::initializeSkybox() {
    ARTIFACT_ZONE();
    m_skyboxProgram = CreateProgram(kSkyboxVertexShader, kSkyboxFragmentShader);
    if (m_skyboxProgram == 0) {
        spdlog::warn("Skybox shader unavailable; background gradient disabled");
        return;
    }

    glGenVertexArrays(1, &m_skyboxVao);
    glBindVertexArray(m_skyboxVao);
    glBindVertexArray(0);
    m_skyboxTopColorLocation = glGetUniformLocation(m_skyboxProgram, "uSkyTopColor");
    m_skyboxHorizonColorLocation = glGetUniformLocation(m_skyboxProgram, "uSkyHorizonColor");
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

}
