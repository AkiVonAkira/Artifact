// Created by Artifact Engine Team
// Base asset class hierarchy for runtime and editor

#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>
#include <cstdint>

namespace Artifact::Assets {
    /**
     * @brief Asset type enumeration
     */
    enum class AssetType {
        Unknown,
        Mesh,
        Texture,
        Material,
        Shader,
        Script,
        Audio,
        Scene,
        Prefab
    };

    /**
     * @brief Map file extensions to asset types
     */
    inline const std::unordered_map<std::string, AssetType> ExtensionMap = {
        {".obj",   AssetType::Mesh},
        {".fbx",   AssetType::Mesh},
        {".gltf",  AssetType::Mesh},

        {".png",   AssetType::Texture},
        {".jpg",   AssetType::Texture},
        {".jpeg",  AssetType::Texture},
        {".tga",   AssetType::Texture},
        {".bmp",   AssetType::Texture},

        {".vert",  AssetType::Shader},
        {".frag",  AssetType::Shader},
        {".glsl",  AssetType::Shader},

        {".lua",   AssetType::Script},

        {".wav",   AssetType::Audio},
        {".mp3",   AssetType::Audio},
        {".ogg",   AssetType::Audio},

        {".scene", AssetType::Scene},
        {".prefab",AssetType::Prefab},
    };

    /**
     * @brief Convert asset type to string
     */
    inline const char* AssetTypeToString(AssetType type) {
        switch (type) {
            case AssetType::Mesh: return "Mesh";
            case AssetType::Texture: return "Texture";
            case AssetType::Material: return "Material";
            case AssetType::Shader: return "Shader";
            case AssetType::Script: return "Script";
            case AssetType::Audio: return "Audio";
            case AssetType::Scene: return "Scene";
            case AssetType::Prefab: return "Prefab";
            default: return "Unknown";
        }
    }

    /**
     * @brief Base asset class with metadata
     *
     * All engine assets derive from this class.
     * Contains common metadata and serialization interface.
     */
    class Asset {
    public:
        using UUID = uint64_t;

        Asset() = default;
        virtual ~Asset() = default;

        // Non-copyable but movable
        Asset(const Asset&) = delete;
        Asset& operator=(const Asset&) = delete;
        Asset(Asset&&) = default;
        Asset& operator=(Asset&&) = default;

        /**
         * @brief Get asset type
         */
        virtual AssetType GetType() const = 0;

        /**
         * @brief Get asset UUID (unique identifier)
         */
        UUID GetUUID() const { return m_uuid; }

        /**
         * @brief Set asset UUID
         */
        void SetUUID(UUID uuid) { m_uuid = uuid; }

        /**
         * @brief Get asset name (display name)
         */
        const std::string& GetName() const { return m_name; }

        /**
         * @brief Set asset name
         */
        void SetName(const std::string& name) { m_name = name; }

        /**
         * @brief Get asset path (relative to project)
         */
        const std::filesystem::path& GetPath() const { return m_path; }

        /**
         * @brief Set asset path
         */
        void SetPath(const std::filesystem::path& path) { m_path = path; }

        /**
         * @brief Get last modified timestamp
         */
        uint64_t GetLastModified() const { return m_lastModified; }

        /**
         * @brief Set last modified timestamp
         */
        void SetLastModified(uint64_t timestamp) { m_lastModified = timestamp; }

        /**
         * @brief Check if asset is loaded
         */
        bool IsLoaded() const { return m_loaded; }

        /**
         * @brief Load asset from file
         * @return True if successful
         */
        virtual bool Load() = 0;

        /**
         * @brief Unload asset from memory
         */
        virtual void Unload() = 0;

        /**
         * @brief Save asset to file
         * @return True if successful
         */
        virtual bool Save() const { return false; }

        /**
         * @brief Get asset size in bytes (when loaded)
         */
        virtual size_t GetMemorySize() const { return 0; }

    protected:
        UUID m_uuid = 0;
        std::string m_name;
        std::filesystem::path m_path;
        uint64_t m_lastModified = 0;
        bool m_loaded = false;
    };

} // namespace Artifact::Assets

