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
        if (auto* entity = ctx.layer.getSelectedEntity()) {
            ImGui::Checkbox("Active", &entity->active);
            ImGui::InputText("Name", &entity->name);

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Position", glm::value_ptr(entity->transform.position), 0.1f);
                ImGui::DragFloat3("Rotation", glm::value_ptr(entity->transform.rotation), 0.5f);
                ImGui::DragFloat3("Scale", glm::value_ptr(entity->transform.scale), 0.05f, 0.0f, 0.0f, "%.2f");
            }

            if (ImGui::CollapsingHeader("Lua Script", ImGuiTreeNodeFlags_DefaultOpen)) {
                const char* current = entity->scriptName.empty() ? "None" : entity->scriptName.c_str();
                if (ImGui::BeginCombo("Script", current)) {
                    bool selected = entity->scriptName.empty();
                    if (ImGui::Selectable("None", selected)) {
                        entity->scriptName.clear();
                    }
                    for (const auto& asset : ctx.state.scriptAssets) {
                        selected = entity->scriptName == asset.name;
                        if (ImGui::Selectable(asset.name.c_str(), selected)) {
                            entity->scriptName = asset.name;
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Checkbox("Auto Reload", &entity->autoReloadScript);
                if (!entity->scriptName.empty()) {
                    ImGui::SameLine();
                    if (ImGui::Button("Run")) {
                        ctx.layer.runEntityScripts(0.0f);
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
