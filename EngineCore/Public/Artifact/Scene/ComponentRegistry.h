#pragma once

#include <entt/entt.hpp>

#include <vector>
#include <string_view>
#include <string>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <utility>

namespace Artifact::Scene {
    using ComponentId = std::size_t;

    enum class ComponentCategory {
        Other,
        Physics,
        Scripting,
        Rendering,
        Core
    };

    struct ComponentMetadata {
        std::function<void(entt::registry &, entt::entity)> remove;
        std::function<void(entt::registry &, entt::entity)> add;
        std::function<bool(entt::registry &, entt::entity)> has;

        bool removable{true}; // Whether the user can remove this component in the editor
        bool editorVisible{true}; // Whether this component shows up in the Add Component UI
        bool allowMultiple{false}; // Whether multiple instances of this component are allowed per entity
        ComponentCategory category{ComponentCategory::Other};
        std::string name; // Display name, e.g. "Camera"
        ComponentId id{};
    };

    class ComponentRegistry {
    public:
        ComponentRegistry() = default;

        // Register a component by providing display name and editor properties.
        // Returns the assigned ComponentId.
        ComponentId registerComponent(std::string_view name,
                                      bool removable,
                                      bool editorVisible,
                                      bool allowMultiple,
                                      ComponentCategory category = ComponentCategory::Other) {
            ComponentMetadata meta{};
            meta.removable = removable;
            meta.editorVisible = editorVisible;
            meta.allowMultiple = allowMultiple;
            meta.category = category;
            meta.name.assign(name.begin(), name.end());
            const ComponentId id = m_components.size();
            meta.id = id;

            m_components.emplace_back(std::move(meta));
            return id;
        }

        // Find metadata by ComponentId
        inline const ComponentMetadata *findById(ComponentId id) const {
            if (id < m_components.size()) return &m_components[id];
            return nullptr;
        }

        // Find metadata by type T (if registered)
        template<typename T>
        const ComponentMetadata *find() const {
            auto it = m_typeToId.find(typeid(T));
            if (it != m_typeToId.end()) {
                return findById(it->second);
            }
            return nullptr;
        }

        // Template helper to register a concrete component type T. This will wire
        // up add/remove/has lambdas that operate on an entt::registry for type T.
        template<typename T>
        ComponentId registerComponent(std::string_view name,
                                      bool removable = true,
                                      bool editorVisible = true,
                                      bool allowMultiple = false,
                                      ComponentCategory category = ComponentCategory::Other) {
            if (auto it = m_typeToId.find(typeid(T)); it != m_typeToId.end()) {
                return it->second;
            }

            ComponentId id = registerComponent(name, removable, editorVisible, allowMultiple, category);

            // Fill in the behavior lambdas for this type
            auto &meta = m_components[id];

            meta.add = [allowMultiple](entt::registry &r, entt::entity e) {
                if (!allowMultiple && r.any_of<T>(e)) return; // already exists
                if (!r.any_of<T>(e)) r.emplace<T>(e);
                else if (allowMultiple) r.emplace<T>(e);
            };

            meta.remove = [](entt::registry &r, entt::entity e) {
                if (r.any_of<T>(e)) r.remove<T>(e);
            };

            meta.has = [](entt::registry &r, entt::entity e) -> bool {
                return r.any_of<T>(e);
            };

            // Remember the mapping from type to id for fast lookup
            m_typeToId[typeid(T)] = id;

            return id;
        }


        // Non-template find: search by predicate (returns first match or nullptr)
        template<typename Predicate>
        inline const ComponentMetadata *findIf(Predicate &&pred) const {
            for (const auto &meta: m_components) {
                if (pred(meta)) return &meta;
            }
            return nullptr;
        }

        const std::vector<ComponentMetadata> &components() const { return m_components; }

    private:
        std::vector<ComponentMetadata> m_components;
        std::unordered_map<std::type_index, ComponentId> m_typeToId;
    };

    // Global registry helpers
    void RegisterDefaultComponents(ComponentRegistry &registry);

    ComponentRegistry &GetGlobalComponentRegistry();
} // namespace Artifact::Scene
