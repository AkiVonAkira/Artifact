#include "Panel.h"
#include "EditorLayer.h"

#include <imgui.h>

namespace Artifact::Editor {
    namespace {
        class ScenePanel : public IEditorPanel {
        public:
            const char *name() const override {
                return "Scene";
            }

            void draw(PanelContext &ctx) override {
                const char *title = ctx.windowTitle ? ctx.windowTitle : "Scene";
                if (!ImGui::Begin(title, ctx.closeable ? ctx.openFlag : nullptr)) {
                    ImGui::End();
                    return;
                }
                const ImVec2 viewportSize = ImGui::GetContentRegionAvail();
                ctx.layer.renderSceneViewport(viewportSize);
                ImGui::End();

                if (ctx.state.showDemoWindow) {
                    ImGui::ShowDemoWindow(&ctx.state.showDemoWindow);
                }
            }
        };
    }

    PanelPtr CreateScenePanel() {
        return std::make_unique<ScenePanel>();
    }
}
