#include "Artifact/Core/Application.h"
#include "Artifact/Editor/EditorLayer.h"

int main() {
    Artifact::Core::ApplicationCreateInfo info;
    info.windowTitle = "Artifact Editor";
    info.enableEditor = true;
    info.enableRuntime = true;

    Artifact::Core::Application app(info);
    Artifact::Editor::EditorLayer editorLayer(app);
    app.run();
    return 0;
}
