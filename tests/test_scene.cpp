#include <catch2/catch_test_macros.hpp>

#include "Artifact/Scene/SceneContext.h"
#include "Artifact/Scene/Prefabs.h"
#include "Artifact/Scene/Components.h"

#include <glm/vec3.hpp>

TEST_CASE("SceneContext assigns sequential ids", "[scene]") {
    Artifact::Scene::SceneContext scene;

    const auto first = Artifact::Scene::CreateEntity(scene, "Player");
    const auto second = Artifact::Scene::CreateEntity(scene, "Enemy");

    auto& registry = scene.registry;
    REQUIRE(registry.valid(first));
    REQUIRE(registry.valid(second));

    const auto& firstId = registry.get<Artifact::Scene::IdComponent>(first);
    const auto& secondId = registry.get<Artifact::Scene::IdComponent>(second);
    REQUIRE(firstId.id == 1);
    REQUIRE(secondId.id == 2);

    const auto& firstName = registry.get<Artifact::Scene::NameComponent>(first);
    const auto& secondName = registry.get<Artifact::Scene::NameComponent>(second);
    REQUIRE(firstName.name == "Player");
    REQUIRE(secondName.name == "Enemy");
}

TEST_CASE("SceneContext reset clears entities", "[scene]") {
    Artifact::Scene::SceneContext scene;
    Artifact::Scene::CreateEntity(scene, "Temp");
    REQUIRE(scene.registry.storage<Artifact::Scene::IdComponent>().size() == 1);

    Artifact::Scene::ResetScene(scene);
    REQUIRE(scene.registry.storage<Artifact::Scene::IdComponent>().size() == 0);
    REQUIRE(scene.nextEntityId == 1);
}

TEST_CASE("Camera prefab applies transform and script", "[scene][prefab]") {
    Artifact::Scene::SceneContext scene;
    Artifact::Scene::Prefabs::CameraPrefabOptions options;
    options.position = {1.0f, 2.0f, 3.0f};
    options.rotation = {45.0f, 10.0f, 0.0f};
    options.scriptName = "CameraController";
    options.autoReloadScript = false;

    const auto entity = Artifact::Scene::Prefabs::CreateMainCamera(scene, options);
    auto& registry = scene.registry;
    REQUIRE(registry.valid(entity));

    const auto& transform = registry.get<Artifact::Scene::TransformComponent>(entity);
    CHECK(transform.transform.position == glm::vec3(1.0f, 2.0f, 3.0f));
    CHECK(transform.transform.rotation == glm::vec3(45.0f, 10.0f, 0.0f));

    const auto& script = registry.get<Artifact::Scene::LuaScriptComponent>(entity);
    CHECK(script.scriptName == "CameraController");
    CHECK_FALSE(script.autoReloadScript);
}

TEST_CASE("Lua actor prefab sets script when provided", "[scene][prefab]") {
    Artifact::Scene::SceneContext scene;
    Artifact::Scene::Prefabs::LuaActorPrefabOptions options;
    options.name = "NPC";
    options.position = {4.0f, 0.5f, -2.0f};
    options.scriptName = "RoamingAI";

    const auto entity = Artifact::Scene::Prefabs::CreateLuaActor(scene, options);
    auto& registry = scene.registry;

    const auto& name = registry.get<Artifact::Scene::NameComponent>(entity);
    CHECK(name.name == "NPC");

    const auto& transform = registry.get<Artifact::Scene::TransformComponent>(entity);
    CHECK(transform.transform.position == glm::vec3(4.0f, 0.5f, -2.0f));

    const auto& script = registry.get<Artifact::Scene::LuaScriptComponent>(entity);
    CHECK(script.scriptName == "RoamingAI");
    CHECK(script.autoReloadScript);
}
