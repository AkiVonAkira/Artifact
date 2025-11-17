#include "Artifact/Core/Application.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    Artifact::Core::ApplicationCreateInfo info;
    info.windowTitle = "Artifact Runtime";
    info.enableEditor = false;
    info.enableRuntime = true;

    Artifact::Core::Application app(info);
    app.run();
    return 0;
}
