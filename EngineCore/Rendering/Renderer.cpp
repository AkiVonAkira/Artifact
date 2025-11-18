#include "Artifact/Rendering/Renderer.h"

#include <glad/glad.h>

#include "Artifact/Core/Instrumentation.h"
#include <spdlog/spdlog.h>

namespace Artifact::Rendering {

Renderer::Renderer() {
    initializeGlState();
}

Renderer::~Renderer() {
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

}
