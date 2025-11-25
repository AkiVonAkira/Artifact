#pragma once

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

    class Mesh {
    public:
        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
    };
}
