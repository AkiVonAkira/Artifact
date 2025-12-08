#pragma once

#include "Asset.h"
#include <string>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

namespace Artifact::Assets {
    struct MeshVertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texcoord;
    };

    class Mesh : public Asset {
    public:
        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;

        // Asset interface
        AssetType GetType() const override { return AssetType::Mesh; }
        bool Load() override { return loadFromBinary(GetPath().string()); }
        void Unload() override { vertices.clear(); indices.clear(); m_loaded = false; }
        bool Save() const override { return saveToBinary(GetPath().string()); }
        size_t GetMemorySize() const override {
            return vertices.size() * sizeof(MeshVertex) + indices.size() * sizeof(uint32_t);
        }

        /// Binary serialization - save mesh to file
        bool saveToBinary(const std::string& filepath) const;

        /// Binary serialization - load mesh from file
        bool loadFromBinary(const std::string& filepath);

        /// Triangulize quads - convert quad faces to triangles
        static void triangulizeQuads(std::vector<MeshVertex>& vertices, std::vector<uint32_t>& indices);
    };
}
