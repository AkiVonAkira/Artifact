// Created by Artifact Engine Team on 2025-11-25.
// Unified asset manager with singleton pattern

#pragma once

#include "Artifact/Core/Singleton.h"
#include "Artifact/Assets/AssetHandle.h"
#include "Artifact/Assets/Mesh.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <functional>

namespace Artifact::Assets {
    class AssetWatcher;

    /**
     * @brief Centralized asset management system
     *
     * Manages loading, caching, and lifecycle of all engine assets:
     * - Meshes (.obj, .fbx, etc.)
     * - Textures (.png, .jpg, etc.)
     * - Shaders (.vert, .frag, etc.)
     * - Scripts (.lua)
     *
     * Features:
     * - Automatic caching and deduplication
     * - Hot-reload support via AssetWatcher
     * - Type-safe asset handles
     * - Path normalization
     */
    class AssetManager : public Core::Singleton<AssetManager> {
        friend class Core::Singleton<AssetManager>;

    public:
        using AssetChangeCallback = std::function<void(const std::filesystem::path &)>;

        /**
         * @brief Initialize the asset manager
         * @param enableHotReload Enable asset watching for hot-reload
         */
        void Initialize(bool enableHotReload = true);

        /**
         * @brief Shutdown the asset manager
         */
        void Shutdown();

        /**
         * @brief Update asset watcher (call once per frame)
         */
        void Update();

        // ========================================================================
        // Mesh Management
        // ========================================================================

        /**
         * @brief Load a mesh from file
         * @param relativePath Path relative to asset root
         * @return Handle to the loaded mesh (or existing cached mesh)
         */
        MeshHandle LoadMesh(const std::string &relativePath);

        /**
         * @brief Get a mesh by handle
         * @param handle Mesh handle
         * @return Pointer to mesh, or nullptr if invalid
         */
        const Mesh *GetMesh(MeshHandle handle) const;

        /**
         * @brief Unload a mesh
         * @param handle Mesh handle
         */
        void UnloadMesh(MeshHandle handle);

        /**
         * @brief Get number of loaded meshes
         */
        size_t GetMeshCount() const { return m_meshes.size(); }

        // ========================================================================
        // Asset Watching & Hot-Reload
        // ========================================================================

        /**
         * @brief Register callback for mesh asset changes
         */
        void OnMeshChanged(AssetChangeCallback callback);

        /**
         * @brief Register callback for shader asset changes
         */
        void OnShaderChanged(AssetChangeCallback callback);

        /**
         * @brief Register callback for script asset changes
         */
        void OnScriptChanged(AssetChangeCallback callback);

        // ========================================================================
        // Path Management
        // ========================================================================

        /**
         * @brief Resolve relative path to absolute path
         */
        std::filesystem::path ResolvePath(const std::string &relativePath) const;

        /**
         * @brief Check if a file exists
         */
        bool FileExists(const std::string &relativePath) const;

        // ========================================================================
        // Statistics
        // ========================================================================

        /**
         * @brief Get total memory used by assets (approximate)
         */
        size_t GetMemoryUsage() const;

        /**
         * @brief Clear all cached assets
         */
        void ClearCache();

    private:
        AssetManager() = default;

        ~AssetManager() = default;

        // Mesh loading implementation
        std::unique_ptr<Mesh> LoadMeshFromFile(const std::filesystem::path &path);

        Mesh ParseOBJ(const std::filesystem::path &path);

        // Asset storage
        struct MeshEntry {
            std::unique_ptr<Mesh> mesh;
            std::filesystem::path path;
            uint64_t lastModified;
        };

        std::unordered_map<MeshHandle::IDType, MeshEntry> m_meshes;
        std::unordered_map<std::string, MeshHandle::IDType> m_meshPathToID;

        // Asset watching
        std::unique_ptr<AssetWatcher> m_watcher;
        std::vector<AssetChangeCallback> m_meshCallbacks;
        std::vector<AssetChangeCallback> m_shaderCallbacks;
        std::vector<AssetChangeCallback> m_scriptCallbacks;

        // ID generation
        MeshHandle::IDType m_nextMeshID = 1;

        bool m_initialized = false;
    };
} // namespace Artifact::Assets
