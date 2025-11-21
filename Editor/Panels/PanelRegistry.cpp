#include "PanelRegistry.h"
#include "PanelFactories.h"

namespace Artifact::Editor {

namespace {
std::vector<PanelDescriptor> BuildRegistry() {
    return {
        {"main-menu", "Main Menu", false, true, false, false, false, [] { return CreateMainMenuPanel(); }},
        {"hierarchy", "Hierarchy", false, true, true, true, true, [] { return CreateHierarchyPanel(); }},
        {"inspector", "Inspector", false, true, true, true, true, [] { return CreateInspectorPanel(); }},
        {"scene", "Scene", false, true, true, true, true, [] { return CreateScenePanel(); }},
        {"console", "Console", true, true, true, true, true, [] { return CreateConsolePanel(); }},
        {"assets", "Assets", true, true, true, true, true, [] { return CreateAssetsPanel(); }},
        {"gameplay-tests", "Gameplay Tests", true, false, true, true, true, [] { return CreateTestRunnerPanel(); }}
    };
}
}

const std::vector<PanelDescriptor>& GetPanelRegistry() {
    static const std::vector<PanelDescriptor> registry = BuildRegistry();
    return registry;
}

const PanelDescriptor* FindPanelDescriptor(std::string_view id) {
    const auto& registry = GetPanelRegistry();
    for (const auto& descriptor : registry) {
        if (descriptor.id == id) {
            return &descriptor;
        }
    }
    return nullptr;
}

}
