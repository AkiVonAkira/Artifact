#include "Panel.h"
#include "EditorLayer.h"

#include <imgui.h>

#include <algorithm>
#include <vector>

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
            const auto entity = ctx.layer.addEntity("GameObject");
            ctx.layer.selectEntity(entity);
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            ctx.layer.removeSelectedEntity();
        }
        ImGui::Separator();

    auto& registry = ctx.state.scene.registry;
        auto view = registry.view<const IdComponent, const NameComponent>();
        std::vector<std::pair<int, entt::entity>> entries;
        entries.reserve(view.size_hint());
        for (auto entity : view) {
            const auto& idComponent = view.get<const IdComponent>(entity);
            entries.emplace_back(idComponent.id, entity);
        }
        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        const auto selected = ctx.layer.getSelectedEntity();
        for (const auto& [id, entity] : entries) {
            const auto& nameComponent = registry.get<const NameComponent>(entity);
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (selected == entity) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(id)), flags, "%s", nameComponent.name.c_str());
            if (ImGui::IsItemClicked()) {
                ctx.layer.selectEntity(entity);
            }
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Duplicate")) {
                    ctx.layer.duplicateEntity(entity);
                }
                if (ImGui::MenuItem("Delete")) {
                    ctx.layer.selectEntity(entity);
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
