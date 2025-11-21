#include "Panel.h"
#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "Artifact/Scene/SceneService.h"
#include "Artifact/Scripting/ScriptAssetService.h"

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
                ctx.layer.assignScriptToSelection(trimmed);
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
        const char* title = ctx.windowTitle ? ctx.windowTitle : "Assets";
        if (!ImGui::Begin(title, ctx.closeable ? ctx.openFlag : nullptr)) {
            ImGui::End();
            return;
        }

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
            const auto selected = ctx.layer.getSelectedEntity();
            auto& registry = ctx.scene.context().registry;
            const auto* scriptComponent = (selected != entt::null && registry.valid(selected)) ? registry.try_get<Scene::LuaScriptComponent>(selected) : nullptr;
            for (const auto& asset : ctx.scripts.scripts()) {
                const bool selectedScript = scriptComponent && scriptComponent->scriptName == asset.name;
                ImGui::Selectable(asset.name.c_str(), selectedScript);
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
