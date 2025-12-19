#include "../Core/Panel.h"
#include "Artifact/Editor/EditorLayer.h"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <variant>

#include "Artifact/Scene/SceneContext.h"
#include "Artifact/Scene/ComponentRegistry.h"
#include "Artifact/Core/Paths.h"

#include <filesystem>
#include <vector>

namespace Artifact::Editor {
    namespace {
        std::vector<std::string> GetAssetFiles(const std::string& subDir, const std::vector<std::string>& extensions) {
            std::vector<std::string> files;
            auto root = Artifact::Core::Paths::AssetsRoot();
            auto path = root;
            if (!subDir.empty()) path /= subDir;

            if (!std::filesystem::exists(path)) return files;

            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    for (const auto& validExt : extensions) {
                        if (ext == validExt) {
                            auto relative = std::filesystem::relative(entry.path(), root);
                            std::string relativeStr = relative.string();
                            std::replace(relativeStr.begin(), relativeStr.end(), '\\', '/');
                            files.push_back("Assets/" + relativeStr);
                            break;
                        }
                    }
                }
            }
            return files;
        }

        struct ComponentHeaderResult {
            bool expanded{false};
            bool removeRequested{false};
        };

        static ComponentHeaderResult DrawComponentHeader(const char *label,
                                                         bool *enabled,
                                                         bool removable) {
            ComponentHeaderResult result{};
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {4.0f, 6.0f});
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8.0f, 6.0f});
            const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen |
                                             ImGuiTreeNodeFlags_Framed |
                                             ImGuiTreeNodeFlags_SpanAvailWidth |
                                             ImGuiTreeNodeFlags_AllowItemOverlap;
            result.expanded = ImGui::CollapsingHeader(label, flags);

            const float headerHeight = ImGui::GetFrameHeight();
            const float xRight = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
            float cursorX = xRight - headerHeight;

            if (removable) {
                ImGui::SameLine(cursorX, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
                if (ImGui::SmallButton("X")) {
                    result.removeRequested = true;
                }
                ImGui::PopStyleColor(2);
                cursorX -= headerHeight;
            }

            if (enabled) {
                ImGui::SameLine(cursorX, 0.0f);
                const float currentY = ImGui::GetCursorPosY();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {2.0f, 2.0f});
                const float toggleHeight = ImGui::GetFrameHeight();
                ImGui::SetCursorPosY(currentY + (headerHeight - toggleHeight) * 0.5f);
                ImGui::Checkbox("##Enabled", enabled);
                ImGui::PopStyleVar();
            }

            ImGui::PopStyleVar(2);
            return result;
        }

        template<typename DrawFn>
        void RenderComponentSection(const Scene::ComponentMetadata *meta,
                                    entt::registry &registry,
                                    entt::entity entity,
                                    DrawFn &&drawFn,
                                    bool *enabledFlag = nullptr) {
            if (!meta || !meta->has || !meta->has(registry, entity)) {
                return;
            }

            ImGui::PushID(meta->name.c_str());
            const auto header = DrawComponentHeader(meta->name.c_str(), enabledFlag, meta->removable && meta->remove);
            if (header.removeRequested && meta->remove) {
                meta->remove(registry, entity);
                ImGui::PopID();
                return;
            }

            if (header.expanded) {
                drawFn();
            }

            ImGui::PopID();
        }
    } // namespace

    class InspectorPanel : public IEditorPanel {
    public:
        const char *name() const override {
            return "Inspector";
        }

        void draw(PanelContext &ctx) override {
            const char *title = ctx.windowTitle ? ctx.windowTitle : "Inspector";
            if (!ImGui::Begin(title, ctx.closeable ? ctx.openFlag : nullptr)) {
                ImGui::End();
                return;
            }
            const auto selected = ctx.layer.getSelectedEntity();
            auto &registry = ctx.layer.sceneManager()->sceneContext().registry;
            auto &componentRegistry = ctx.layer.sceneManager()->sceneContext().components();

            if (selected != entt::null && registry.valid(selected)) {
                auto *active = registry.try_get<Scene::ActiveComponent>(selected);
                auto *name = registry.try_get<Scene::NameComponent>(selected);
                if (active || name) {
                    const float frameHeight = ImGui::GetFrameHeight();
                    const float spacing = active ? ImGui::GetStyle().ItemSpacing.x : 0.0f;
                    const float toggleSpace = active ? frameHeight : 0.0f;
                    const float available = ImGui::GetContentRegionAvail().x;
                    const float nameWidth = std::max(0.0f, available - toggleSpace - spacing);
                    const float startX = ImGui::GetCursorPosX();
                    const float startY = ImGui::GetCursorPosY();

                    if (name) {
                        if (nameWidth > 0.0f) {
                            ImGui::PushItemWidth(nameWidth);
                        }
                        ImGui::InputTextWithHint("##EntityName", "GameObject", &name->name);
                        if (nameWidth > 0.0f) {
                            ImGui::PopItemWidth();
                        }
                    }

                    if (active) {
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(startX + nameWidth + spacing);
                        ImGui::SetCursorPosY(startY);
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {2.0f, 2.0f});
                        const float toggleHeight = ImGui::GetFrameHeight();
                        ImGui::SetCursorPosY(startY + (frameHeight - toggleHeight) * 0.5f);
                        ImGui::Checkbox("##EntityActive", &active->active);
                        ImGui::PopStyleVar();
                    }
                }

                if (const auto *transformMeta = componentRegistry.find<Scene::TransformComponent>()) {
                    RenderComponentSection(transformMeta, registry, selected, [&, selected] {
                        auto *transform = registry.try_get<Scene::TransformComponent>(selected);
                        if (!transform) {
                            return;
                        }
                        ImGui::DragFloat3("Position", glm::value_ptr(transform->transform.position), 0.1f);
                        ImGui::DragFloat3("Rotation", glm::value_ptr(transform->transform.rotation), 0.5f);
                        ImGui::DragFloat3("Scale", glm::value_ptr(transform->transform.scale), 0.05f, 0.0f, 0.0f,
                                          "%.2f");
                    });
                }

                if (const auto *cameraMeta = componentRegistry.find<Scene::CameraComponent>()) {
                    auto *camera = registry.try_get<Scene::CameraComponent>(selected);
                    RenderComponentSection(cameraMeta, registry, selected, [&, camera] {
                        if (!camera) {
                            return;
                        }
                        ImGui::Checkbox("Primary", &camera->isPrimary);

                        float fov = camera->camera.fieldOfView();
                        if (ImGui::DragFloat("Field of View", &fov, 0.5f, 1.0f, 179.0f)) {
                            camera->camera.setPerspective(fov,
                                                          camera->camera.aspectRatio(),
                                                          camera->camera.nearPlane(),
                                                          camera->camera.farPlane());
                        }

                        float nearPlane = camera->camera.nearPlane();
                        float farPlane = camera->camera.farPlane();
                        if (ImGui::DragFloat("Near Clip", &nearPlane, 0.01f, 0.01f, farPlane - 0.01f)) {
                            camera->camera.setPerspective(fov,
                                                          camera->camera.aspectRatio(),
                                                          nearPlane,
                                                          farPlane);
                        }
                        if (ImGui::DragFloat("Far Clip", &farPlane, 1.0f, nearPlane + 0.1f, 10000.0f)) {
                            camera->camera.setPerspective(fov,
                                                          camera->camera.aspectRatio(),
                                                          nearPlane,
                                                          farPlane);
                        }
                    }, camera ? &camera->enabled : nullptr);
                }

                if (const auto *meshMeta = componentRegistry.find<Scene::MeshComponent>()) {
                    auto *mesh = registry.try_get<Scene::MeshComponent>(selected);
                    RenderComponentSection(meshMeta, registry, selected, [&, mesh] {
                        if (!mesh) {
                            return;
                        }

                        // Show only filename in preview, but store full path
                        std::string previewName = mesh->meshPath.empty() ? "None" : std::filesystem::path(mesh->meshPath).filename().string();
                        
                        if (ImGui::BeginCombo("Mesh", previewName.c_str())) {
                             auto files = GetAssetFiles("", {".obj"});
                             
                             for (const auto& file : files) {
                                 bool isSelected = (mesh->meshPath == file);
                                 std::string displayName = std::filesystem::path(file).filename().string();
                                 if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                                     mesh->meshPath = file;
                                 }
                                 if (isSelected) ImGui::SetItemDefaultFocus();
                             }
                             ImGui::EndCombo();
                        }
                        
                        // Drag and drop target for the combo box
                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
                                std::string path = (const char*)payload->Data;
                                std::string ext = std::filesystem::path(path).extension().string();
                                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                                if (ext == ".obj") {
                                    mesh->meshPath = path;
                                }
                            }
                            ImGui::EndDragDropTarget();
                        }
                    });
                }

                if (const auto *materialMeta = componentRegistry.find<Scene::MaterialComponent>()) {
                    auto *material = registry.try_get<Scene::MaterialComponent>(selected);
                    RenderComponentSection(materialMeta, registry, selected, [&, material] {
                        if (!material) {
                            return;
                        }
                        ImGui::ColorEdit4("Color", glm::value_ptr(material->color));

                        if (ImGui::BeginCombo("Albedo Texture", material->albedoTexture.c_str())) {
                             auto files = GetAssetFiles("", {".png", ".jpg", ".jpeg", ".tga", ".bmp"});

                             if (ImGui::Selectable("None", material->albedoTexture.empty())) {
                                 material->albedoTexture = "";
                             }

                             for (const auto& file : files) {
                                 bool isSelected = (material->albedoTexture == file);
                                 if (ImGui::Selectable(file.c_str(), isSelected)) {
                                     material->albedoTexture = file;
                                 }
                                 if (isSelected) ImGui::SetItemDefaultFocus();
                             }
                             ImGui::EndCombo();
                        }

                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
                                std::string path = (const char*)payload->Data;
                                std::string ext = std::filesystem::path(path).extension().string();
                                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") {
                                    material->albedoTexture = path;
                                }
                            }
                            ImGui::EndDragDropTarget();
                        }

                        ImGui::Checkbox("Double Sided", &material->doubleSided);
                    });
                }

                ImGui::Separator();
                if (ImGui::Button("+ Add Component")) {
                    ImGui::OpenPopup("AddComponentPopup");
                }

                static std::string addComponentSearch;
                if (ImGui::BeginPopup("AddComponentPopup")) {
                    ImGui::InputText("Search", &addComponentSearch);
                    ImGui::Separator();

                    const auto &components = componentRegistry.components();
                    for (const auto &meta: components) {
                        if (!meta.editorVisible) {
                            continue;
                        }
                        if (!meta.allowMultiple && meta.has && meta.has(registry, selected)) {
                            continue;
                        }
                        if (!addComponentSearch.empty()) {
                            std::string lowerName = meta.name;
                            std::string lowerSearch = addComponentSearch;
                            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) {
                                return static_cast<char>(std::tolower(c));
                            });
                            std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(),
                                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                            if (lowerName.find(lowerSearch) == std::string::npos) {
                                continue;
                            }
                        }

                        if (ImGui::Selectable(meta.name.c_str())) {
                            if (meta.add) {
                                meta.add(registry, selected);
                            }
                            ImGui::CloseCurrentPopup();
                            break;
                        }
                    }

                    ImGui::EndPopup();
                }
            } else {
                ImGui::TextUnformatted("Select an entity to inspect its components.");
            }

            ImGui::End();
        }
    };

    PanelPtr CreateInspectorPanel() {
        return std::make_unique<InspectorPanel>();
    }
} // namespace Artifact::Editor
