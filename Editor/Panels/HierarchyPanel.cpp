#include "Panel.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <vector>

#include "EditorLayer.h"
#include "Artifact/Scene/SceneService.h"
#include "Artifact/Scene/SceneManager.h"
#include "Artifact/Core/Macros.h"

namespace Artifact::Editor {
    namespace {
        constexpr const char *kHierarchyPayload = "HIERARCHY_ENTITY";

        bool IsDescendant(const entt::registry &registry, entt::entity ancestor, entt::entity candidate) {
            if (ancestor == entt::null || candidate == entt::null) {
                return false;
            }
            auto *transform = registry.try_get<Scene::TransformComponent>(candidate);
            while (transform && transform->parent != entt::null) {
                if (transform->parent == ancestor) {
                    return true;
                }
                transform = registry.try_get<Scene::TransformComponent>(transform->parent);
            }
            return false;
        }

        void HandleDrop(PanelContext &ctx, entt::entity dropped, entt::entity newParent) {
            auto &registry = ctx.scene.context().registry;
            if (dropped == entt::null || !registry.valid(dropped)) {
                return;
            }
            if (dropped == newParent) {
                return;
            }

            auto *droppedTransform = registry.try_get<Scene::TransformComponent>(dropped);
            if (!droppedTransform) {
                return;
            }

            if (newParent == entt::null) {
                ctx.scene.setParent(dropped, entt::null);
                return;
            }

            if (!registry.valid(newParent)) {
                return;
            }

            if (IsDescendant(registry, dropped, newParent)) {
                const entt::entity droppedParent = droppedTransform->parent;
                ctx.scene.setParent(newParent, droppedParent);
                ctx.scene.setParent(dropped, newParent);
            } else {
                ctx.scene.setParent(dropped, newParent);
            }
        }

        class HierarchyPanel : public IEditorPanel {
        public:
            const char *name() const override {
                return "Hierarchy";
            }

            void draw(PanelContext &ctx) override {
                const char *title = ctx.windowTitle ? ctx.windowTitle : "Hierarchy";
                if (!ImGui::Begin(title, ctx.closeable ? ctx.openFlag : nullptr)) {
                    ImGui::End();
                    return;
                }
                const ImRect dropRect = ImGui::GetCurrentWindow()->InnerRect;
                if (ImGui::BeginDragDropTargetCustom(dropRect, ImGui::GetID("HierarchyWindowDropTarget"))) {
                    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(kHierarchyPayload)) {
                        const entt::entity dropped = *static_cast<const entt::entity *>(payload->Data);
                        HandleDrop(ctx, dropped, entt::null);
                    }
                    ImGui::EndDragDropTarget();
                }
                if (ImGui::Button("Create Empty")) {
                    const auto entity = ctx.layer.addEntity("GameObject");
                    ctx.layer.selectEntity(entity);
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete")) {
                    ctx.layer.removeSelectedEntity();
                }
                ImGui::Separator();

                auto &registry = ctx.scene.context().registry;
                auto view = registry.view<const Scene::TransformComponent, const Scene::NameComponent>();
                std::vector<entt::entity> roots;
                for (auto entity: view) {
                    const auto &transform = view.get<const Scene::TransformComponent>(entity);
                    if (transform.parent == entt::null) {
                        roots.push_back(entity);
                    }
                }
                // keep insertion order (Entt iteration order) to mimic Unity's bottom insertion

                for (auto entity: roots) {
                    DrawEntityNode(ctx, entity);
                }
                ImGui::End();
            }

        private:
            void DrawEntityNode(PanelContext &ctx, entt::entity entity) {
                auto &registry = ctx.scene.context().registry;
                const auto &nameComponent = registry.get<const Scene::NameComponent>(entity);
                const auto &transform = registry.get<const Scene::TransformComponent>(entity);

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (transform.firstChild == entt::null) {
                    flags |= ImGuiTreeNodeFlags_Leaf;
                }
                if (ctx.layer.getSelectedEntity() == entity) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                const bool open = ImGui::TreeNodeEx(reinterpret_cast<void *>(static_cast<intptr_t>(entity)), flags,
                                                    "%s", nameComponent.name.c_str());
                if (ImGui::IsItemClicked()) {
                    ctx.layer.selectEntity(entity);
                }

                if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload(kHierarchyPayload, &entity, sizeof(entt::entity));
                    ImGui::TextUnformatted(nameComponent.name.c_str());
                    ImGui::EndDragDropSource();
                }

                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(kHierarchyPayload)) {
                        const entt::entity dropped = *static_cast<const entt::entity *>(payload->Data);
                        HandleDrop(ctx, dropped, entity);
                    }
                    ImGui::EndDragDropTarget();
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

                if (open) {
                    entt::entity child = transform.firstChild;
                    while (child != entt::null) {
                        DrawEntityNode(ctx, child);
                        child = registry.get<const Scene::TransformComponent>(child).nextSibling;
                    }
                    ImGui::TreePop();
                }
            }
        };
    }

    auto CreateHierarchyPanel() -> PanelPtr {
        return std::make_unique<HierarchyPanel>();
    }
}
