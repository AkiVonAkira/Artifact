#include "Artifact/Core/Application.h"
#include "Artifact/Editor/EditorLayer.h"
#include "Artifact/Core/MemoryChecker.h"

int main() {
    Artifact::Core::MemoryChecker::setEnabled(true);

    Artifact::Core::ApplicationCreateInfo info;
    info.windowTitle = "Artifact Editor";
    info.enableEditor = true;
    info.enableRuntime = true;

    Artifact::Core::Application app(info);
    Artifact::Editor::EditorLayer editorLayer(app);
    app.run();
    return 0;
}
