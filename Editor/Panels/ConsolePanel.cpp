#include "Panel.h"
#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace Artifact::Editor {
namespace {
class ConsolePanel : public IEditorPanel {
public:
    const char* name() const override {
        return "Console";
    }

    void draw(PanelContext& ctx) override {
        ImGui::Begin("Console");
        if (ImGui::Button("Clear")) {
            ctx.state.consoleMessages.clear();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Auto Scroll", &ctx.state.consoleAutoScroll);
        ImGui::Separator();
        ctx.state.consoleFilter.Draw("Filter");

        ImGui::BeginChild("ConsoleScroll", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& message : ctx.state.consoleMessages) {
            if (ctx.state.consoleFilter.PassFilter(message.text.c_str())) {
                ImGui::TextColored(message.color, "%s", message.text.c_str());
            }
        }
        if (ctx.state.consoleAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        if (ImGui::InputText("##luaInput", &ctx.state.luaInputBuffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
            ctx.layer.executeLuaCommand(ctx.state.luaInputBuffer);
            ctx.state.luaInputBuffer.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Run Lua")) {
            ctx.layer.executeLuaCommand(ctx.state.luaInputBuffer);
            ctx.state.luaInputBuffer.clear();
        }
        ImGui::End();
    }
};
}

PanelPtr CreateConsolePanel() {
    return std::make_unique<ConsolePanel>();
}

}
