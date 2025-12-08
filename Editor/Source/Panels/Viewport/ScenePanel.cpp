#include "../Core/Panel.h"
#include "Artifact/Editor/EditorLayer.h"
#include "Artifact/Editor/EditorViewportManager.h"
#include "Artifact/Scene/SceneContext.h"

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

                // Use EditorViewportManager for rendering
                auto* viewportManager = ctx.layer.viewportManager();
                if (viewportManager) {
                    // Get runtime scene info from layer (temporary until fully migrated)
                    Scene::SceneContext* runtimeScene = nullptr;
                    bool runtimeActive = false;

                    // For now, we'll pass nullptr for runtime scene
                    // TODO: Add proper accessors to EditorLayer for runtime state
                    viewportManager->renderSceneViewport(
                        viewportSize,
                        &ctx.scene,
                        &ctx.playMode,
                        runtimeScene,
                        runtimeActive
                    );
                }

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
