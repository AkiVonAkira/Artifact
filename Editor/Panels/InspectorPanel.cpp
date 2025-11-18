#include "Panel.h"
#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <algorithm>

namespace Artifact::Editor {
namespace {
class InspectorPanel : public IEditorPanel {
public:
    const char* name() const override {
        return "Inspector";
    }

    void draw(PanelContext& ctx) override {
        ImGui::Begin("Inspector");
        const auto selected = ctx.layer.getSelectedEntity();
        auto& registry = ctx.state.scene.registry;
        if (selected != entt::null && registry.valid(selected)) {
            if (auto* active = registry.try_get<ActiveComponent>(selected)) {
                ImGui::Checkbox("Active", &active->active);
            }
            if (auto* name = registry.try_get<NameComponent>(selected)) {
                ImGui::InputText("Name", &name->name);
            }

            if (auto* transform = registry.try_get<TransformComponent>(selected)) {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::DragFloat3("Position", glm::value_ptr(transform->transform.position), 0.1f);
                    ImGui::DragFloat3("Rotation", glm::value_ptr(transform->transform.rotation), 0.5f);
                    ImGui::DragFloat3("Scale", glm::value_ptr(transform->transform.scale), 0.05f, 0.0f, 0.0f, "%.2f");
                }
            }

            if (auto* script = registry.try_get<LuaScriptComponent>(selected)) {
                if (ImGui::CollapsingHeader("Lua Script", ImGuiTreeNodeFlags_DefaultOpen)) {
                    const char* current = script->scriptName.empty() ? "None" : script->scriptName.c_str();
                    if (ImGui::BeginCombo("Script", current)) {
                        bool selectedScript = script->scriptName.empty();
                        if (ImGui::Selectable("None", selectedScript)) {
                            script->scriptName.clear();
                        }
                        for (const auto& asset : ctx.state.scriptAssets) {
                            selectedScript = script->scriptName == asset.name;
                            if (ImGui::Selectable(asset.name.c_str(), selectedScript)) {
                                script->scriptName = asset.name;
                            }
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::Checkbox("Enabled", &script->enabled);
                    ImGui::Checkbox("Auto Reload", &script->autoReloadScript);

                    const LuaScriptAsset* scriptAsset = nullptr;
                    if (!script->scriptName.empty()) {
                        auto assetIt = std::find_if(ctx.state.scriptAssets.begin(), ctx.state.scriptAssets.end(), [script](const LuaScriptAsset& asset) {
                            return asset.name == script->scriptName;
                        });
                        if (assetIt != ctx.state.scriptAssets.end()) {
                            scriptAsset = &(*assetIt);
                        }
                    }

                    if (scriptAsset && !scriptAsset->properties.empty()) {
                        ImGui::Separator();
                        ImGui::TextUnformatted("Properties");
                        for (const auto& property : scriptAsset->properties) {
                            ImGui::PushID(property.name.c_str());
                            auto fieldIt = script->serializedFields.find(property.name);
                            auto commitChange = [&](const LuaPropertyValue& value) {
                                script->serializedFields[property.name] = value;
                                ctx.layer.onScriptPropertiesModified(selected);
                            };

                            switch (property.type) {
                                case LuaPropertyType::Boolean: {
                                    bool value = property.boolDefault;
                                    if (fieldIt != script->serializedFields.end() && std::holds_alternative<bool>(fieldIt->second)) {
                                        value = std::get<bool>(fieldIt->second);
                                    }
                                    if (ImGui::Checkbox(property.name.c_str(), &value)) {
                                        commitChange(value);
                                    }
                                    break;
                                }
                                case LuaPropertyType::String: {
                                    std::string value = property.stringDefault;
                                    if (fieldIt != script->serializedFields.end() && std::holds_alternative<std::string>(fieldIt->second)) {
                                        value = std::get<std::string>(fieldIt->second);
                                    }
                                    if (ImGui::InputText(property.name.c_str(), &value)) {
                                        commitChange(value);
                                    }
                                    break;
                                }
                                case LuaPropertyType::Number:
                                default: {
                                    double current = property.numberDefault;
                                    if (fieldIt != script->serializedFields.end() && std::holds_alternative<double>(fieldIt->second)) {
                                        current = std::get<double>(fieldIt->second);
                                    }
                                    float value = static_cast<float>(current);
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
                }
            }
        } else {
            ImGui::TextUnformatted("Select an entity to inspect its components.");
        }
        ImGui::End();
    }
};
}

PanelPtr CreateInspectorPanel() {
    return std::make_unique<InspectorPanel>();
}

}
