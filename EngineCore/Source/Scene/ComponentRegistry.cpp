#include "Artifact/Scene/ComponentRegistry.h"

#include "Artifact/Scene/Components.h"

namespace Artifact::Scene {
    static ComponentRegistry g_registry;

    ComponentRegistry &GetGlobalComponentRegistry() {
        return g_registry;
    }

    void RegisterDefaultComponents(ComponentRegistry &registry) {
        // Core components: not removable, not multi, usually not directly addable by user
        registry.registerComponent<IdComponent>(
            "ID",
            /*removable*/ false,
            /*editorVisible*/ false,
            /*allowMultiple*/ false,
            ComponentCategory::Core);

        registry.registerComponent<NameComponent>(
            "Name",
            /*removable*/ false,
            /*editorVisible*/ false,
            /*allowMultiple*/ false,
            ComponentCategory::Core);

        registry.registerComponent<ActiveComponent>(
            "Active",
            /*removable*/ false,
            /*editorVisible*/ false,
            /*allowMultiple*/ false,
            ComponentCategory::Core);

        registry.registerComponent<TransformComponent>(
            "Transform",
            /*removable*/ false,
            /*editorVisible*/ false, // always created with entity
            /*allowMultiple*/ false,
            ComponentCategory::Core);

        // Rendering
        registry.registerComponent<CameraComponent>(
            "Camera",
            /*removable*/ true,
            /*editorVisible*/ true,
            /*allowMultiple*/ false,
            ComponentCategory::Rendering);

        registry.registerComponent<MeshComponent>(
            "Mesh",
            /*removable*/ true,
            /*editorVisible*/ true,
            /*allowMultiple*/ false,
            ComponentCategory::Rendering);

        registry.registerComponent<MaterialComponent>(
            "Material",
            /*removable*/ true,
            /*editorVisible*/ true,
            /*allowMultiple*/ false,
            ComponentCategory::Rendering);
    }
} // namespace Artifact::Scene
