#pragma once

#include "Panel.h"

namespace Artifact::Editor {
    PanelPtr CreateMainMenuPanel();
    PanelPtr CreateHierarchyPanel();
    PanelPtr CreateInspectorPanel();
    PanelPtr CreateScenePanel();
    PanelPtr CreateConsolePanel();
    PanelPtr CreateAssetsPanel();
    PanelPtr CreateTestRunnerPanel();
}
