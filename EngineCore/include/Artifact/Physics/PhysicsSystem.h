#pragma once

#include <memory>

namespace Artifact::Physics {

class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void step2D(float dt);
    void step3D(float dt);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
