#pragma once
#include "EditorState.h"

namespace Artifact::Editor {

class EditorLayer;

struct PanelContext {
    EditorState& state;
    EditorLayer& layer;
};

class IEditorPanel {
public:
    virtual ~IEditorPanel() = default;
    virtual const char* name() const = 0;
    virtual void draw(PanelContext& ctx) = 0;
};

using PanelPtr = std::unique_ptr<IEditorPanel>;

}

