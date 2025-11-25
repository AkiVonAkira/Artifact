#pragma once

#include <memory>

#include "Artifact/Core/Singleton.h"

namespace Artifact::Physics {
    class PhysicsSystem;

    /**
     * @brief Central physics system managing 2D and 3D physics simulation
     *
     * PhysicsManager is a singleton that handles all physics operations:
     * - Box2D for 2D physics
     * - Jolt Physics for 3D physics
     * - Physics world stepping and updates
     *
     * Access via PhysicsManager::Instance()
     */
    class PhysicsManager : public Core::Singleton<PhysicsManager> {
    public:
        PhysicsManager();

        ~PhysicsManager() override;

        // Physics simulation
        void Step2D(float dt);

        void Step3D(float dt);

        // Direct access to underlying system (for advanced use cases)
        PhysicsSystem *GetPhysicsSystem() { return m_physicsSystem.get(); }

    private:
        std::unique_ptr<PhysicsSystem> m_physicsSystem;
    };
} // namespace Artifact::Physics
