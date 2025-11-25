#include "Artifact/Physics/PhysicsManager.h"

#include "Artifact/Physics/PhysicsSystem.h"
#include "Artifact/Core/LogManager.h"
#include "Artifact/Core/Macros.h"

namespace Artifact::Physics {
    PhysicsManager::PhysicsManager() {
        LOG_INFO("PhysicsManager: Initializing");
        m_physicsSystem = std::make_unique<PhysicsSystem>();
        LOG_INFO("PhysicsManager: Initialization complete");
    }

    PhysicsManager::~PhysicsManager() {
        LOG_INFO("PhysicsManager: Shutting down");
        m_physicsSystem.reset();
    }

    void PhysicsManager::Step2D(float dt) {
        if (!m_physicsSystem) {
            LOG_ERROR("PhysicsManager: Cannot step 2D physics, system not initialized");
            return;
        }
        m_physicsSystem->Step2D(dt);
    }

    void PhysicsManager::Step3D(float dt) {
        if (!m_physicsSystem) {
            LOG_ERROR("PhysicsManager: Cannot step 3D physics, system not initialized");
            return;
        }
        m_physicsSystem->Step3D(dt);
    }
} // namespace Artifact::Physics
