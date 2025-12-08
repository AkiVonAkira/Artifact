#include "Artifact/Assets/Mesh.h"
#include "Artifact/Core/LogManager.h"
#include <fstream>

namespace Artifact::Assets {

    // Binary format:
    // uint32_t magic number (0x4D455348 = "MESH")
    // uint32_t version (1)
    // uint32_t vertex count
    // uint32_t index count
    // MeshVertex[vertex count]
    // uint32_t[index count]

    constexpr uint32_t MESH_MAGIC = 0x4D455348; // "MESH"
    constexpr uint32_t MESH_VERSION = 1;

    bool Mesh::saveToBinary(const std::string& filepath) const {
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR("Mesh::saveToBinary: Failed to open file for writing: {}", filepath);
            return false;
        }

        // Write header
        uint32_t magic = MESH_MAGIC;
        uint32_t version = MESH_VERSION;
        uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
        uint32_t indexCount = static_cast<uint32_t>(indices.size());

        file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        file.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
        file.write(reinterpret_cast<const char*>(&indexCount), sizeof(indexCount));

        // Write vertices
        if (vertexCount > 0) {
            file.write(reinterpret_cast<const char*>(vertices.data()), vertexCount * sizeof(MeshVertex));
        }

        // Write indices
        if (indexCount > 0) {
            file.write(reinterpret_cast<const char*>(indices.data()), indexCount * sizeof(uint32_t));
        }

        if (!file.good()) {
            LOG_ERROR("Mesh::saveToBinary: Error writing to file: {}", filepath);
            return false;
        }

        LOG_INFO("Mesh::saveToBinary: Saved mesh to {} ({} vertices, {} indices)",
                 filepath, vertexCount, indexCount);
        return true;
    }

    bool Mesh::loadFromBinary(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR("Mesh::loadFromBinary: Failed to open file: {}", filepath);
            return false;
        }

        // Read header
        uint32_t magic, version, vertexCount, indexCount;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        file.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
        file.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));

        if (magic != MESH_MAGIC) {
            LOG_ERROR("Mesh::loadFromBinary: Invalid magic number in file: {}", filepath);
            return false;
        }

        if (version != MESH_VERSION) {
            LOG_WARN("Mesh::loadFromBinary: Version mismatch (expected {}, got {}) in file: {}",
                     MESH_VERSION, version, filepath);
        }

        // Read vertices
        vertices.resize(vertexCount);
        if (vertexCount > 0) {
            file.read(reinterpret_cast<char*>(vertices.data()), vertexCount * sizeof(MeshVertex));
        }

        // Read indices
        indices.resize(indexCount);
        if (indexCount > 0) {
            file.read(reinterpret_cast<char*>(indices.data()), indexCount * sizeof(uint32_t));
        }

        if (!file.good()) {
            LOG_ERROR("Mesh::loadFromBinary: Error reading from file: {}", filepath);
            return false;
        }

        LOG_INFO("Mesh::loadFromBinary: Loaded mesh from {} ({} vertices, {} indices)",
                 filepath, vertexCount, indexCount);
        return true;
    }

    void Mesh::triangulizeQuads(std::vector<MeshVertex>& vertices, std::vector<uint32_t>& indices) {
        // Check if indices count is divisible by 4 (quad faces)
        if (indices.size() % 4 != 0) {
            LOG_WARN("Mesh::triangulizeQuads: Index count not divisible by 4, assuming already triangulated");
            return;
        }

        std::vector<uint32_t> newIndices;
        newIndices.reserve((indices.size() / 4) * 6); // Each quad becomes 2 triangles

        // Process each quad (4 indices)
        for (size_t i = 0; i < indices.size(); i += 4) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            uint32_t i3 = indices[i + 3];

            // First triangle (0, 1, 2)
            newIndices.push_back(i0);
            newIndices.push_back(i1);
            newIndices.push_back(i2);

            // Second triangle (0, 2, 3)
            newIndices.push_back(i0);
            newIndices.push_back(i2);
            newIndices.push_back(i3);
        }

        indices = std::move(newIndices);
        LOG_INFO("Mesh::triangulizeQuads: Converted {} quads to {} triangles",
                 indices.size() / 6, indices.size() / 3);
    }

} // namespace Artifact::Assets

