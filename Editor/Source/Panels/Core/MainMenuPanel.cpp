#include "Panel.h"
#include "Artifact/Editor/EditorLayer.h"

#include <imgui.h>


namespace Artifact::Editor {
    namespace {
        class MainMenuPanel : public IEditorPanel {
        public:
            const char *name() const override {
                return "MainMenu";
            }

            void draw(PanelContext &ctx) override {
                if (!ImGui::BeginMainMenuBar()) {
                    return;
                }

                auto &layer = ctx.layer;
                auto &state = ctx.state;

                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                        layer.createDefaultScene();
                        layer.pushConsoleMessage("New scene created", spdlog::level::info);
                    }
                    if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, false)) {
                        layer.pushConsoleMessage("Scene saving not implemented yet", spdlog::level::warn);
                    }
                    if (ImGui::MenuItem("Exit", "Ctrl+Q")) {
                        layer.requestExit();
                    }
                    ImGui::EndMenu();
                }

            if (ImGui::BeginMenu("Window")) {
                if (ImGui::BeginMenu("Layout")) {
                    if (ImGui::MenuItem("Reset to Default")) {
                        layer.resetLayout();
                    }
                    ImGui::EndMenu();
                }

                for (const auto &descriptor: layer.panelDescriptors()) {
                    if (!descriptor.visibleInWindowMenu) {
                        continue;
                    }
                    if (ImGui::MenuItem(descriptor.label.c_str())) {
                        layer.openPanel(descriptor.id);
                    }
                }

                ImGui::MenuItem("ImGui Demo", nullptr, &state.showDemoWindow);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Open Docs")) {
                    layer.pushConsoleMessage("Docs live in docs/ - open Architecture.md for an overview.",
                                             spdlog::level::info);
                }
                ImGui::EndMenu();
            }

            layer.drawPlayControlsInMenuBar();
            ImGui::EndMainMenuBar();
        }
    };
    }


    PanelPtr CreateMainMenuPanel() {
        return std::make_unique<MainMenuPanel>();
    }
}
