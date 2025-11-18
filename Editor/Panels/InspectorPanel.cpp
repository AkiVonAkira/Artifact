#include "Panel.h"
#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>

namespace Artifact::Editor {
namespace {
class InspectorPanel : public IEditorPanel {
public:
    const char* name() const override {
        return "Inspector";
    }

    void draw(PanelContext& ctx) override {
        ImGui::Begin("Inspector");
        const auto selected = ctx.layer.getSelectedEntity();
        auto& registry = ctx.state.scene.registry;
        if (selected != entt::null && registry.valid(selected)) {
            if (auto* active = registry.try_get<ActiveComponent>(selected)) {
                ImGui::Checkbox("Active", &active->active);
            }
            if (auto* name = registry.try_get<NameComponent>(selected)) {
                ImGui::InputText("Name", &name->name);
            }

            if (auto* transform = registry.try_get<TransformComponent>(selected)) {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::DragFloat3("Position", glm::value_ptr(transform->transform.position), 0.1f);
                    ImGui::DragFloat3("Rotation", glm::value_ptr(transform->transform.rotation), 0.5f);
                    ImGui::DragFloat3("Scale", glm::value_ptr(transform->transform.scale), 0.05f, 0.0f, 0.0f, "%.2f");
                }
            }

            if (auto* script = registry.try_get<LuaScriptComponent>(selected)) {
                if (ImGui::CollapsingHeader("Lua Script", ImGuiTreeNodeFlags_DefaultOpen)) {
                    const char* current = script->scriptName.empty() ? "None" : script->scriptName.c_str();
                    if (ImGui::BeginCombo("Script", current)) {
                        bool selectedScript = script->scriptName.empty();
                        if (ImGui::Selectable("None", selectedScript)) {
                            script->scriptName.clear();
                        }
                        for (const auto& asset : ctx.state.scriptAssets) {
                            selectedScript = script->scriptName == asset.name;
                            if (ImGui::Selectable(asset.name.c_str(), selectedScript)) {
                                script->scriptName = asset.name;
                            }
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::Checkbox("Auto Reload", &script->autoReloadScript);
                    if (!script->scriptName.empty()) {
                        ImGui::SameLine();
                        if (ImGui::Button("Run")) {
                            ctx.layer.runEntityScripts(0.0f);
                        }
                    }
                }
            }
        } else {
            ImGui::TextUnformatted("Select an entity to inspect its components.");
        }
        ImGui::End();
    }
};
}

PanelPtr CreateInspectorPanel() {
    return std::make_unique<InspectorPanel>();
}

}
