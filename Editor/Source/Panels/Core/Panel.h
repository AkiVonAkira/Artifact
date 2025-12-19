#pragma once
#include <memory>
#include <vector>

#include "Artifact/Editor/EditorState.h"

namespace Artifact {
    namespace Core {
        class PlayModeController;
    }
}

namespace Artifact::Editor {
    class EditorLayer;

    /**
     * @brief Context passed to editor panels
     *
     * Panels access scene through layer.sceneManager()->sceneContext()
     * and playMode through layer.playMode()
     */
    struct PanelContext {
        EditorState &state;
        EditorLayer &layer;

        // Legacy reference
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
