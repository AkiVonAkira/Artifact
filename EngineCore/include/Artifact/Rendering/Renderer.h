#pragma once

#include <memory>

namespace Artifact::Rendering {

struct FrameContext {
    float deltaSeconds{0.0f};
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    void beginFrame(const FrameContext& ctx);
    void endFrame();

private:
    void initializeGlState();
};

}
