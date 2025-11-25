#include "Artifact/Physics/PhysicsSystem.h"

#include "Artifact/Core/Instrumentation.h"

#include <box2d/box2d.h>
#include <Jolt/Jolt.h>

#include <cstdarg>
#include <cstdio>
#include <spdlog/spdlog.h>

namespace Artifact::Physics {
    struct PhysicsSystem::Impl {
        Impl() {
            ARTIFACT_ZONE();
            // Initialize Box2D world or other physics-related members here.
        }

        ~Impl() {
            ARTIFACT_ZONE();
            // Cleanup Box2D world or other physics-related members here.
        }
    };

    PhysicsSystem::PhysicsSystem()
        : m_impl(std::make_unique<Impl>()) {
        ARTIFACT_ZONE();
        JPH::RegisterDefaultAllocator();
        JPH::Trace = [](const char *fmt, ...) {
            va_list args;
            va_start(args, fmt);
            char buffer[1024];
            vsnprintf(buffer, sizeof(buffer), fmt, args);
            va_end(args);
            spdlog::debug("[Jolt] {}", buffer);
        };
    }

    PhysicsSystem::~PhysicsSystem() = default;

    void PhysicsSystem::Step2D(float dt) {
        ARTIFACT_ZONE();
        (void) dt; // TODO: integrate Box2D physics system update loop.
    }

    void PhysicsSystem::Step3D(float dt) {
        ARTIFACT_ZONE();
        (void) dt; // TODO: integrate Jolt physics system update loop.
    }
}
