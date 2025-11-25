#include "Artifact/Scripting/LuaBindings.h"

#include "Artifact/Scripting/LuaContext.h"

#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <sol/sol.hpp>

namespace Artifact::Scripting {
    LuaBindings::LuaBindings(LuaContext &context)
        : m_context(context) {
    }

    void LuaBindings::registerInputBindings(std::function<bool(int)> keyQuery) {
        m_keyQuery = std::move(keyQuery);
        auto &lua = m_context.state();
        sol::table input = ensureInputTable();
        input.set_function("IsKeyDown", [this](int key) {
            return m_keyQuery ? m_keyQuery(key) : false;
        });

        sol::object keyTableObject = input.raw_get<sol::object>("Key");
        sol::table keyTable;
        if (keyTableObject.valid() && keyTableObject.get_type() == sol::type::table) {
            keyTable = keyTableObject;
        } else {
            keyTable = lua.create_table();
            input.raw_set("Key", keyTable);
        }

        keyTable["W"] = GLFW_KEY_W;
        keyTable["A"] = GLFW_KEY_A;
        keyTable["S"] = GLFW_KEY_S;
        keyTable["D"] = GLFW_KEY_D;
        keyTable["Q"] = GLFW_KEY_Q;
        keyTable["E"] = GLFW_KEY_E;
        keyTable["SPACE"] = GLFW_KEY_SPACE;
        keyTable["LEFT_SHIFT"] = GLFW_KEY_LEFT_SHIFT;
        keyTable["RIGHT_SHIFT"] = GLFW_KEY_RIGHT_SHIFT;
    }

    void LuaBindings::registerTimeBindings() {
        ensureTimeTable();
    }

    void LuaBindings::registerEntityBindings() {
        sol::table entity = ensureEntityTable();

        entity.set_function("Translate", [this](float x, float y, float z) {
            if (auto *transform = getActiveTransform()) {
                transform->transform.position += glm::vec3{x, y, z};
            }
        });

        entity.set_function("SetPosition", [this](float x, float y, float z) {
            if (auto *transform = getActiveTransform()) {
                transform->transform.position = {x, y, z};
            }
        });

        entity.set_function("GetPosition", [this]() {
            auto &luaState = m_context.state();
            glm::vec3 position{0.0f};
            if (auto *transform = getActiveTransform()) {
                position = transform->transform.position;
            }
            return luaState.create_table_with(
                "x", position.x,
                "y", position.y,
                "z", position.z);
        });

        entity.set_function("SetRotation", [this](float x, float y, float z) {
            if (auto *transform = getActiveTransform()) {
                transform->transform.rotation = {x, y, z};
            }
        });

        entity.set_function("AddRotation", [this](float x, float y, float z) {
            if (auto *transform = getActiveTransform()) {
                transform->transform.rotation += glm::vec3{x, y, z};
            }
        });

        entity.set_function("GetName", [this]() {
            if (const auto *name = getActiveName()) {
                return name->name;
            }
            return std::string{};
        });
    }

    void LuaBindings::setRegistry(entt::registry *registry) {
        m_registry = registry;
    }

    void LuaBindings::setActiveEntity(entt::entity entity) {
        m_activeEntity = entity;
    }

    void LuaBindings::clearActiveEntity() {
        m_activeEntity = entt::null;
    }

    void LuaBindings::updateDeltaTime(float deltaTime) {
        sol::table timeTable = ensureTimeTable();
        timeTable.raw_set("deltaTime", deltaTime);
    }

    void LuaBindings::updateEntityMetadata(const std::string &name, int id) {
        sol::table entity = ensureEntityTable();
        entity.raw_set("__name", name);
        entity.raw_set("__id", id);
    }

    void LuaBindings::clearEntityMetadata() {
        sol::table entity = ensureEntityTable();
        entity.raw_set("__name", sol::lua_nil);
        entity.raw_set("__id", sol::lua_nil);
    }

    sol::table LuaBindings::ensureInputTable() {
        return ensureTable("Input");
    }

    sol::table LuaBindings::ensureEntityTable() {
        return ensureTable("Entity");
    }

    sol::table LuaBindings::ensureTimeTable() {
        return ensureTable("Time");
    }

    sol::table LuaBindings::ensureTable(const char *name) {
        auto &lua = m_context.state();
        sol::table globals = lua.globals();
        sol::object existing = globals.raw_get<sol::object>(name);
        if (existing.valid() && existing.get_type() == sol::type::table) {
            return existing;
        }

        sol::table created = lua.create_table();
        globals.raw_set(name, created);
        return created;
    }

    Scene::TransformComponent *LuaBindings::getActiveTransform() {
        if (!m_registry || m_activeEntity == entt::null) {
            return nullptr;
        }
        return m_registry->try_get<Scene::TransformComponent>(m_activeEntity);
    }

    const Scene::NameComponent *LuaBindings::getActiveName() const {
        if (!m_registry || m_activeEntity == entt::null) {
            return nullptr;
        }
        return m_registry->try_get<Scene::NameComponent>(m_activeEntity);
    }
}
