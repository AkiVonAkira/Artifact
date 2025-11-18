#pragma once

#include <functional>
#include <string>

#include <entt/entt.hpp>

#include "Artifact/Scene/Components.h"

namespace Artifact::Scripting {

class LuaContext;

class LuaBindings {
public:
    explicit LuaBindings(LuaContext& context);

    void registerInputBindings(std::function<bool(int)> keyQuery);
    void registerTimeBindings();
    void registerEntityBindings();

    void setRegistry(entt::registry* registry);
    void setActiveEntity(entt::entity entity);
    void clearActiveEntity();

    void updateDeltaTime(float deltaTime);
    void updateEntityMetadata(const std::string& name, int id);
    void clearEntityMetadata();

private:
    LuaContext& m_context;
    std::function<bool(int)> m_keyQuery;
    entt::registry* m_registry{nullptr};
    entt::entity m_activeEntity{entt::null};

    void ensureInputTable();
    void ensureEntityTable();
    void ensureTimeTable();

    Scene::TransformComponent* getActiveTransform();
    const Scene::NameComponent* getActiveName() const;
};

}
