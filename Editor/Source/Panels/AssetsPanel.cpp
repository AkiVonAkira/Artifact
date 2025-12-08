#include "Panel.h"

#include <imgui.h>

namespace Artifact::Editor {
    namespace {
        class AssetsPanel : public IEditorPanel {
        public:
            const char *name() const override {
                return "Assets";
            }

            void draw(PanelContext &ctx) override {
                const char *title = ctx.windowTitle ? ctx.windowTitle : "Assets";
                if (!ImGui::Begin(title, ctx.closeable ? ctx.openFlag : nullptr)) {
                    ImGui::End();
                    return;
                }

                ImGui::TextDisabled("Assets Panel - Placeholder");
                ImGui::Separator();
                ImGui::TextWrapped("This panel was previously used for Lua script management. "
                                   "Lua scripting support has been removed from the engine.");
                ImGui::Spacing();
                ImGui::TextWrapped("Future versions may add asset browsing and management features here.");

                ImGui::End();
            }
        };
    }

    PanelPtr CreateAssetsPanel() {
        return std::make_unique<AssetsPanel>();
    }
}
