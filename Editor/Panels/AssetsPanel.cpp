#include "Panel.h"
#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace Artifact::Editor {
namespace {

void DrawCreateScriptPopup(PanelContext& ctx) {
    if (ImGui::BeginPopupModal("CreateLuaScriptPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("New Lua script name");
        ImGui::InputText("##LuaScriptName", &ctx.state.newScriptName);
        ImGui::TextDisabled("Use letters, numbers, and '_' only");
        if (!ctx.state.scriptCreationError.empty()) {
            ImGui::TextColored(EditorLayer::kErrorColor, "%s", ctx.state.scriptCreationError.c_str());
        }

        if (ImGui::Button("Create", ImVec2(120.0f, 0.0f))) {
            const std::string trimmed = ctx.layer.trimString(ctx.state.newScriptName);
            if (ctx.layer.createLuaScript(trimmed)) {
                if (auto* entity = ctx.layer.getSelectedEntity()) {
                    entity->scriptName = trimmed;
                }
                ctx.state.scriptCreationError.clear();
                ctx.state.newScriptName.clear();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
            ctx.state.scriptCreationError.clear();
            ctx.state.newScriptName.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

class AssetsPanel : public IEditorPanel {
public:
    const char* name() const override {
        return "Assets";
    }

    void draw(PanelContext& ctx) override {
        ImGui::Begin("Assets");
        const float buttonSize = ImGui::GetFrameHeight();
        bool open = ImGui::TreeNodeEx("Lua Scripts", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - buttonSize);
        if (ImGui::SmallButton("+##CreateLuaScript")) {
            ctx.state.newScriptName.clear();
            ctx.state.scriptCreationError.clear();
            ImGui::OpenPopup("CreateLuaScriptPopup");
        }
        DrawCreateScriptPopup(ctx);

        if (open) {
            for (const auto& asset : ctx.state.scriptAssets) {
                bool selected = false;
                if (auto* entity = ctx.layer.getSelectedEntity()) {
                    selected = entity->scriptName == asset.name;
                }
                ImGui::Selectable(asset.name.c_str(), selected);
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    ctx.layer.assignScriptToSelection(asset.name);
                }
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Assign to selection")) {
                        ctx.layer.assignScriptToSelection(asset.name);
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::TreePop();
        }
        ImGui::End();
    }
};
}

PanelPtr CreateAssetsPanel() {
    return std::make_unique<AssetsPanel>();
}

}
