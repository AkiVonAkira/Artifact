#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Artifact/Scripting/LuaBindings.h"
#include "Artifact/Scripting/LuaContext.h"
#include "Artifact/Scene/SceneContext.h"

#include <glm/vec3.hpp>
#include <sol/sol.hpp>

namespace {
sol::table RequireGlobalTable(sol::state& state, const char* name)
{
	sol::table globals = state.globals();
    sol::object tableObject = globals.raw_get<sol::object>(name);
	REQUIRE(tableObject.valid());
	REQUIRE(tableObject.get_type() == sol::type::table);
	return tableObject.as<sol::table>();
}
}

TEST_CASE("LuaBindings exposes entity transforms", "[lua][bindings]") {
    Artifact::Scripting::LuaContext luaContext;
    Artifact::Scripting::LuaBindings bindings(luaContext);

    Artifact::Scene::SceneContext scene;
    const auto entity = Artifact::Scene::CreateEntity(scene, "TestEntity");
    auto& registry = scene.registry;

    auto& transform = registry.get<Artifact::Scene::TransformComponent>(entity);
    transform.transform.position = {10.0f, 5.0f, -3.0f};

    bindings.registerEntityBindings();
    bindings.setRegistry(&registry);
    bindings.setActiveEntity(entity);

    auto& state = luaContext.state();
    state.open_libraries(sol::lib::base);

    sol::protected_function_result result = state.safe_script(R"(
        local pos = Entity.GetPosition()
        Entity.Translate(-2, 1, 3)
        Entity.SetRotation(0, 90, 0)
        Entity.AddRotation(0, 5, 0)
        Entity.SetPosition(pos.x * 2, pos.y, pos.z)
    )", sol::script_pass_on_error);

    REQUIRE(result.valid());

    CHECK(transform.transform.position == glm::vec3(20.0f, 5.0f, -3.0f));
    CHECK(transform.transform.rotation == glm::vec3(0.0f, 95.0f, 0.0f));
}

TEST_CASE("LuaBindings metadata updates", "[lua][bindings]") {
    Artifact::Scripting::LuaContext luaContext;
    Artifact::Scripting::LuaBindings bindings(luaContext);
    bindings.registerEntityBindings();

    bindings.updateEntityMetadata("Actor", 42);

    auto& state = luaContext.state();
    sol::table entity = RequireGlobalTable(state, "Entity");
    CHECK(entity["__name"].get<std::string>() == "Actor");
    CHECK(entity["__id"].get<int>() == 42);

    bindings.clearEntityMetadata();
    CHECK(entity["__name"].get_type() == sol::type::none);
    CHECK(entity["__id"].get_type() == sol::type::none);
}

TEST_CASE("LuaBindings updates delta time", "[lua][bindings]") {
    Artifact::Scripting::LuaContext luaContext;
    Artifact::Scripting::LuaBindings bindings(luaContext);
    bindings.registerTimeBindings();

    bindings.updateDeltaTime(0.016f);

    auto& state = luaContext.state();
    sol::table timeTable = RequireGlobalTable(state, "Time");
    CHECK(timeTable["deltaTime"].get<float>() == Catch::Approx(0.016f));
}
