#pragma once

#include <cstdint>
#include <memory>

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

private:
    void initializeGlState();
    void ensureSceneFramebuffer(int width, int height);
    void destroySceneFramebuffer();

    unsigned int m_sceneFramebuffer{0};
    unsigned int m_sceneColor{0};
    unsigned int m_sceneDepth{0};
    int m_sceneWidth{0};
    int m_sceneHeight{0};
};

}
