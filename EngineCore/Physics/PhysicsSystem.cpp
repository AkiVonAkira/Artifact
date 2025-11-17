#include "Artifact/Physics/PhysicsSystem.h"

#include "Artifact/Core/Instrumentation.h"

#include <box2d/box2d.h>
#include <Jolt/Jolt.h>

namespace Artifact::Physics {

struct PhysicsSystem::Impl {
    b2World world2D{b2Vec2{0.0f, -9.81f}};
};

PhysicsSystem::PhysicsSystem()
    : m_impl(std::make_unique<Impl>()) {
    ARTIFACT_ZONE();
    JPH::RegisterDefaultAllocator();
    JPH::Trace = [](const char* msg) {
        (void)msg;
    };
}

PhysicsSystem::~PhysicsSystem() = default;

void PhysicsSystem::step2D(float dt) {
    ARTIFACT_ZONE();
    m_impl->world2D.Step(dt, 8, 3);
}

void PhysicsSystem::step3D(float dt) {
    ARTIFACT_ZONE();
    (void)dt; // TODO: integrate Jolt physics system update loop.
}

}
