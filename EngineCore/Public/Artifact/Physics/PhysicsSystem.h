#pragma once

#include <memory>

namespace Artifact::Physics {
    class PhysicsSystem {
    public:
        PhysicsSystem();

        ~PhysicsSystem();

        void Step2D(float dt);

        void Step3D(float dt);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
