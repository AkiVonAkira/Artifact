#include "Panel.h"
#include "EditorLayer.h"
#include "Artifact/Core/LogManager.h"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace Artifact::Editor {
    namespace {
        // Convert LogEntry::Level to ImGui color
        ImVec4 GetLogColor(Artifact::Core::LogEntry::Level level) {
            switch (level) {
                case Artifact::Core::LogEntry::Level::Trace: return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
                case Artifact::Core::LogEntry::Level::Debug: return ImVec4(0.6f, 0.8f, 1.0f, 1.0f); // Light blue
                case Artifact::Core::LogEntry::Level::Info: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
                case Artifact::Core::LogEntry::Level::Warn: return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
                case Artifact::Core::LogEntry::Level::Error: return ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red
                case Artifact::Core::LogEntry::Level::Critical: return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Bright red
                default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            }
        }

        class ConsolePanel : public IEditorPanel {
        public:
            const char *name() const override {
                return "Console";
            }

            void draw(PanelContext &ctx) override {
                const char *title = ctx.windowTitle ? ctx.windowTitle : "Console";
                if (!ImGui::Begin(title, ctx.closeable ? ctx.openFlag : nullptr)) {
                    ImGui::End();
                    return;
                }

                auto vectorSink = Artifact::Core::LogManager::Instance().GetVectorSink();

                // If the console panel is being opened for the first time this run, clear the in-memory logs
                if (vectorSink && ctx.state.consoleClearOnOpen) {
                    vectorSink->Clear();
                    ctx.state.consoleClearOnOpen = false;
                }

                if (ImGui::Button("Clear")) {
                    if (vectorSink) {
                        vectorSink->Clear();
                    }
                }
                ImGui::SameLine();
                ImGui::Checkbox("Auto Scroll", &ctx.state.consoleAutoScroll);
                ImGui::Separator();
                ctx.state.consoleFilter.Draw("Filter");

                ImGui::BeginChild("ConsoleScroll", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2), true,
                                  ImGuiWindowFlags_HorizontalScrollbar);

                if (vectorSink) {
                    auto logs = vectorSink->GetLogs();
                    for (const auto &entry: logs) {
                        std::string fullMessage = "[" + entry.timestamp + "] " + entry.message;
                        if (ctx.state.consoleFilter.PassFilter(fullMessage.c_str())) {
                            ImVec4 color = GetLogColor(entry.level);
                            ImGui::TextColored(color, "%s", fullMessage.c_str());
                        }
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
