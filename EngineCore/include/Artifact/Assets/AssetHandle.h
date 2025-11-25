// Created by Artifact Engine Team on 2025-11-25.
// Type-safe asset handle system

#pragma once

#include <cstdint>
#include <functional>

namespace Artifact::Assets {
    /**
     * @brief Type-safe asset handle
     * @tparam T Asset type (Mesh, Texture, Shader, etc.)
     */
    template<typename T>
    class AssetHandle {
    public:
        using IDType = uint64_t;

        AssetHandle() : m_id(0) {
        }

        explicit AssetHandle(IDType id) : m_id(id) {
        }

        IDType ID() const { return m_id; }
        bool IsValid() const { return m_id != 0; }

        bool operator==(const AssetHandle &other) const { return m_id == other.m_id; }
        bool operator!=(const AssetHandle &other) const { return m_id != other.m_id; }
        bool operator<(const AssetHandle &other) const { return m_id < other.m_id; }

        explicit operator bool() const { return IsValid(); }

    private:
        IDType m_id;
    };

    // Common asset handle types
    class Mesh;
    class Texture;
    class Shader;
    class Script;

    using MeshHandle = AssetHandle<Mesh>;
    using TextureHandle = AssetHandle<Texture>;
    using ShaderHandle = AssetHandle<Shader>;
    using ScriptHandle = AssetHandle<Script>;
} // namespace Artifact::Assets

// Hash support for using handles in unordered_map/set
namespace std {
    template<typename T>
    struct hash<Artifact::Assets::AssetHandle<T> > {
        size_t operator()(const Artifact::Assets::AssetHandle<T> &handle) const {
            return std::hash<typename Artifact::Assets::AssetHandle<T>::IDType>{}(handle.ID());
        }
    };
}
