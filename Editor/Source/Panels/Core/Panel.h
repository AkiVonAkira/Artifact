#pragma once
#include <memory>
#include <vector>

#include "Artifact/Editor/EditorState.h"

namespace Artifact {
    namespace Scene {
        class SceneService;
    }

    namespace Core {
        class PlayModeController;
    }
}

namespace Artifact::Editor {
    class EditorLayer;

    /**
     * @brief Context passed to editor panels
     *
     * Note: Panels should prefer using singleton managers directly:
     * - SceneManager::Instance() for scene operations
     * - ScriptManager::Instance() for scripting
     * - RenderManager::Instance() for rendering
     * - etc.
     *
     * Legacy scene and playMode references kept for gradual migration.
     */
    struct PanelContext {
        EditorState &state;
        EditorLayer &layer;

        // Legacy references (TODO: Remove after panels migrate to singletons)
        Scene::SceneService &scene;
        Core::PlayModeController &playMode;

        const char *windowTitle = nullptr;
        const char *windowId = nullptr;
        bool *openFlag = nullptr;
        bool hasWindow = true;
        bool closeable = true;
    };

    class IEditorPanel {
    public:
        virtual ~IEditorPanel() = default;

        virtual const char *name() const = 0;

        virtual void draw(PanelContext &ctx) = 0;
    };

    using PanelPtr = std::unique_ptr<IEditorPanel>;
}
