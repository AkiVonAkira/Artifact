#include "Panel.h"
#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <variant>

#include "Artifact/Scene/SceneService.h"
#include "Artifact/Scripting/ScriptAssetService.h"
#include "Artifact/Scene/ComponentRegistry.h"

namespace Artifact::Editor {
namespace {
struct ComponentHeaderResult {
    bool expanded{false};
    bool removeRequested{false};
};

static ComponentHeaderResult DrawComponentHeader(const char* label,
                                                 bool* enabled,
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
void RenderComponentSection(const Scene::ComponentMetadata* meta,
                            entt::registry& registry,
                            entt::entity entity,
                            DrawFn&& drawFn,
                            bool* enabledFlag = nullptr) {
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
    const char* name() const override {
        return "Inspector";
    }

    void draw(PanelContext& ctx) override {
        const char* title = ctx.windowTitle ? ctx.windowTitle : "Inspector";
        if (!ImGui::Begin(title, ctx.closeable ? ctx.openFlag : nullptr)) {
            ImGui::End();
            return;
        }
        const auto selected = ctx.layer.getSelectedEntity();
        auto& registry = ctx.scene.context().registry;
        auto& componentRegistry = ctx.scene.context().components();

        if (selected != entt::null && registry.valid(selected)) {
            auto* active = registry.try_get<Scene::ActiveComponent>(selected);
            auto* name = registry.try_get<Scene::NameComponent>(selected);
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

            if (const auto* transformMeta = componentRegistry.find<Scene::TransformComponent>()) {
                RenderComponentSection(transformMeta, registry, selected, [&, selected] {
                    auto* transform = registry.try_get<Scene::TransformComponent>(selected);
                    if (!transform) {
                        return;
                    }
                    ImGui::DragFloat3("Position", glm::value_ptr(transform->transform.position), 0.1f);
                    ImGui::DragFloat3("Rotation", glm::value_ptr(transform->transform.rotation), 0.5f);
                    ImGui::DragFloat3("Scale", glm::value_ptr(transform->transform.scale), 0.05f, 0.0f, 0.0f, "%.2f");
                });
            }

            if (const auto* cameraMeta = componentRegistry.find<Scene::CameraComponent>()) {
                auto* camera = registry.try_get<Scene::CameraComponent>(selected);
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

            if (const auto* scriptMeta = componentRegistry.find<Scene::LuaScriptComponent>()) {
                auto* script = registry.try_get<Scene::LuaScriptComponent>(selected);
                RenderComponentSection(scriptMeta, registry, selected, [&, script, selected] {
                    if (!script) {
                        return;
                    }
                    const char* current = script->scriptName.empty() ? "None" : script->scriptName.c_str();
                    if (ImGui::BeginCombo("Script", current)) {
                        bool selectedScript = script->scriptName.empty();
                        if (ImGui::Selectable("None", selectedScript)) {
                            script->scriptName.clear();
                        }
                        for (const auto& asset : ctx.scripts.scripts()) {
                            selectedScript = script->scriptName == asset.name;
                            if (ImGui::Selectable(asset.name.c_str(), selectedScript)) {
                                ctx.layer.assignScriptToSelection(asset.name);
                            }
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::Checkbox("Auto Reload", &script->autoReloadScript);

                    const auto* scriptAsset = ctx.scripts.findByName(script->scriptName);
                    if (scriptAsset && !scriptAsset->properties.empty()) {
                        ImGui::Separator();
                        ImGui::TextUnformatted("Properties");
                        for (const auto& property : scriptAsset->properties) {
                            ImGui::PushID(property.name.c_str());
                            auto fieldIt = script->serializedFields.find(property.name);
                            auto commitChange = [&](const Scene::LuaPropertyValue& value) {
                                script->serializedFields[property.name] = value;
                                ctx.layer.onScriptPropertiesModified(selected);
                            };

                            switch (property.type) {
                                case Scripting::ScriptPropertyType::Boolean: {
                                    bool value = property.boolDefault;
                                    if (fieldIt != script->serializedFields.end() && std::holds_alternative<bool>(fieldIt->second)) {
                                        value = std::get<bool>(fieldIt->second);
                                    }
                                    if (ImGui::Checkbox(property.name.c_str(), &value)) {
                                        commitChange(value);
                                    }
                                    break;
                                }
                                case Scripting::ScriptPropertyType::String: {
                                    std::string value = property.stringDefault;
                                    if (fieldIt != script->serializedFields.end() && std::holds_alternative<std::string>(fieldIt->second)) {
                                        value = std::get<std::string>(fieldIt->second);
                                    }
                                    if (ImGui::InputText(property.name.c_str(), &value)) {
                                        commitChange(value);
                                    }
                                    break;
                                }
                                case Scripting::ScriptPropertyType::Number:
                                default: {
                                    double currentProp = property.numberDefault;
                                    if (fieldIt != script->serializedFields.end() && std::holds_alternative<double>(fieldIt->second)) {
                                        currentProp = std::get<double>(fieldIt->second);
                                    }
                                    float value = static_cast<float>(currentProp);
                                    const float minValue = property.minValue ? static_cast<float>(*property.minValue) : -std::numeric_limits<float>::infinity();
                                    const float maxValue = property.maxValue ? static_cast<float>(*property.maxValue) : std::numeric_limits<float>::infinity();
                                    bool changed = false;
                                    if (property.minValue && property.maxValue) {
                                        changed = ImGui::SliderFloat(property.name.c_str(), &value, minValue, maxValue);
                                    } else {
                                        const float speed = 0.1f;
                                        changed = ImGui::DragFloat(property.name.c_str(), &value, speed, minValue, maxValue);
                                    }
                                    if (changed) {
                                        commitChange(static_cast<double>(value));
                                    }
                                    break;
                                }
                            }
                            ImGui::PopID();
                        }
                    }
                }, script ? &script->enabled : nullptr);
            }

            ImGui::Separator();
            if (ImGui::Button("+ Add Component")) {
                ImGui::OpenPopup("AddComponentPopup");
            }

            static std::string addComponentSearch;
            if (ImGui::BeginPopup("AddComponentPopup")) {
                ImGui::InputText("Search", &addComponentSearch);
                ImGui::Separator();

                const auto& components = componentRegistry.components();
                for (const auto& meta : components) {
                    if (!meta.editorVisible) {
                        continue;
                    }
                    if (!meta.allowMultiple && meta.has && meta.has(registry, selected)) {
                        continue;
                    }
                    if (!addComponentSearch.empty()) {
                        std::string lowerName = meta.name;
                        std::string lowerSearch = addComponentSearch;
                        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
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
