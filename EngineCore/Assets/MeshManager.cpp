#include "Artifact/Assets/MeshManager.h"
#include "Artifact/Core/Paths.h"

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>

namespace Artifact::Assets {
namespace {
Mesh ParseObj(const std::string& relativePath) {
    Mesh mesh;
    const auto path = Core::Paths::ResolveAssetPath(relativePath);
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::error("Failed to open OBJ: {}", path.string());
        return mesh;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        if (token == "v") {
            glm::vec3 p{};
            iss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (token == "vt") {
            glm::vec2 t{};
            iss >> t.x >> t.y;
            texcoords.push_back(t);
        } else if (token == "vn") {
            glm::vec3 n{};
            iss >> n.x >> n.y >> n.z;
            normals.push_back(glm::normalize(n));
        } else if (token == "f") {
            int faceIndex = 0;
            std::string vertSpec;
            while (iss >> vertSpec && faceIndex < 3) {
                int v = 0, vt = 0, vn = 0;
                sscanf_s(vertSpec.c_str(), "%d/%d/%d", &v, &vt, &vn);
                MeshVertex vertex{};
                if (v > 0 && static_cast<size_t>(v) <= positions.size()) {
                    vertex.position = positions[v - 1];
                }
                if (vn > 0 && static_cast<size_t>(vn) <= normals.size()) {
                    vertex.normal = normals[vn - 1];
                }
                if (vt > 0 && static_cast<size_t>(vt) <= texcoords.size()) {
                    vertex.texcoord = texcoords[vt - 1];
                }
                mesh.indices.push_back(static_cast<uint32_t>(mesh.vertices.size()));
                mesh.vertices.push_back(vertex);
                ++faceIndex;
            }
        }
    }

    if (mesh.vertices.empty()) {
        spdlog::warn("OBJ has no vertices: {}", path.string());
    }

    return mesh;
}
}

const Mesh* MeshManager::loadMesh(const std::string& path) {
    if (path.empty()) {
        return nullptr;
    }
    const std::string normalized = Core::Paths::ResolveAssetPath(path).string();
    if (auto it = m_meshes.find(normalized); it != m_meshes.end()) {
        return it->second.get();
    }

    auto mesh = std::make_unique<Mesh>(ParseObj(normalized));
    const Mesh* ptr = mesh.get();
    m_meshes.emplace(normalized, std::move(mesh));
    return ptr;
}

}
