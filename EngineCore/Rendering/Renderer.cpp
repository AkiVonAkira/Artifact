#include "Artifact/Rendering/Renderer.h"

#include "Artifact/Core/Instrumentation.h"

#include <glad/glad.h>

namespace Artifact::Rendering {

Renderer::Renderer() {
    initializeGlState();
}

Renderer::~Renderer() = default;

void Renderer::beginFrame(const FrameContext& ctx) {
    ARTIFACT_ZONE();
    (void)ctx;
    glViewport(0, 0, 1600, 900);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {
    ARTIFACT_ZONE();
}

void Renderer::initializeGlState() {
    ARTIFACT_ZONE();
    glEnable(GL_DEPTH_TEST);
}

}
