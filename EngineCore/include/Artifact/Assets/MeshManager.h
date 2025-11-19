#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Artifact/Assets/Mesh.h"

namespace Artifact::Assets {

class MeshManager {
public:
    const Mesh* loadMesh(const std::string& relativePath);

private:
    std::unordered_map<std::string, std::unique_ptr<Mesh>> m_meshes;
};

} // namespace Artifact::Assets
