#pragma once

#include <functional>
#include <memory>
#include <string>

#include <entt/entt.hpp>

#include "Artifact/Scene/SceneContext.h"

namespace Artifact::Scripting {
class ScriptAssetService;
}

namespace Artifact::Scene {

class SceneService {
public:
    using SelectionChangedCallback = std::function<void(entt::entity)>;

    SceneService();
    ~SceneService();

    SceneContext& context();
    const SceneContext& context() const;

    entt::entity createEntity(const std::string& name);
    entt::entity createDefaultCamera();
    entt::entity createDefaultLuaActor();
    entt::entity duplicateEntity(entt::entity source);
    void destroyEntity(entt::entity entity);

    void selectEntity(entt::entity entity);
    entt::entity selectedEntity() const;
    void onSelectionChanged(SelectionChangedCallback callback);

    void resetScene();
    void createDefaultScene(const Scripting::ScriptAssetService* scripts = nullptr);

private:
    SceneContext m_scene;
    entt::entity m_selected{entt::null};
    SelectionChangedCallback m_selectionChanged;
};

using SceneServicePtr = std::shared_ptr<SceneService>;

} // namespace Artifact::Scene
