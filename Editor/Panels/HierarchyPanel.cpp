#include "Panel.h"
#include "EditorLayer.h"

#include <imgui.h>

namespace Artifact::Editor {
namespace {
class HierarchyPanel : public IEditorPanel {
public:
    const char* name() const override {
        return "Hierarchy";
    }

    void draw(PanelContext& ctx) override {
        ImGui::Begin("Hierarchy");
        if (ImGui::Button("Create Empty")) {
            auto& entity = ctx.layer.addEntity("GameObject");
            ctx.state.selectedEntity = entity.id;
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            ctx.layer.removeSelectedEntity();
        }
        ImGui::Separator();

        for (auto& entity : ctx.state.entities) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (ctx.state.selectedEntity == entity.id) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(entity.id)), flags, "%s", entity.name.c_str());
            if (ImGui::IsItemClicked()) {
                ctx.state.selectedEntity = entity.id;
            }
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Duplicate")) {
                    auto duplicate = entity;
                    duplicate.id = 0;
                    duplicate.name += " Copy";
                    auto& newEntity = ctx.layer.addEntity(duplicate.name);
                    newEntity.transform = duplicate.transform;
                    newEntity.scriptName = duplicate.scriptName;
                    newEntity.autoReloadScript = duplicate.autoReloadScript;
                }
                if (ImGui::MenuItem("Delete")) {
                    ctx.state.selectedEntity = entity.id;
                    ctx.layer.removeSelectedEntity();
                }
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }
};
}

auto CreateHierarchyPanel() -> PanelPtr {
    return std::make_unique<HierarchyPanel>();
}

}
