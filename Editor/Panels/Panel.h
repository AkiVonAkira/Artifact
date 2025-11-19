#pragma once
#include <memory>

#include "EditorState.h"

namespace Artifact {
namespace Scene { class SceneService; }
namespace Scripting { class ScriptAssetService; class LuaRuntimeSystem; }
namespace Core { class PlayModeController; }
}

namespace Artifact::Editor {

class EditorLayer;

struct PanelContext {
    EditorState& state;
    EditorLayer& layer;
    Scene::SceneService& scene;
    Scripting::ScriptAssetService& scripts;
    Scripting::LuaRuntimeSystem& luaRuntime;
    Core::PlayModeController& playMode;
};

class IEditorPanel {
public:
    virtual ~IEditorPanel() = default;
    virtual const char* name() const = 0;
    virtual void draw(PanelContext& ctx) = 0;
};

using PanelPtr = std::unique_ptr<IEditorPanel>;

}
