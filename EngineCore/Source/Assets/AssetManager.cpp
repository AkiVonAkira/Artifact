#include "Artifact/Assets/AssetManager.h"
#include "Artifact/Assets/AssetWatcher.h"
#include "Artifact/Core/Paths.h"
#include "Artifact/Core/LogManager.h"
#include "Artifact/Core/Macros.h"

#include <glm/glm.hpp>
#include <fstream>
#include <sstream>

namespace Artifact::Assets {
    void AssetManager::Initialize(bool enableHotReload) {
        if (m_initialized) {
            LOG_WARN("AssetManager already initialized");
            return;
        }

        LOG_INFO("Initializing AssetManager...");

        if (enableHotReload) {
            m_watcher = std::make_unique<AssetWatcher>();

            // Watch for mesh changes
            auto modelPath = Core::Paths::ProjectRoot() / "Assets" / "Models";
            if (std::filesystem::exists(modelPath)) {
                m_watcher->watch(modelPath, [this](const AssetChange &change) {
                    if (change.path.extension() == ".obj" || change.path.extension() == ".fbx") {
                        LOG_INFO("Mesh asset changed: {}", change.path.string());
                        for (auto &callback: m_meshCallbacks) {
                            callback(change.path);
                        }
                    }
                });
            }

            // Watch for shader changes
            auto shaderPath = Core::Paths::ProjectRoot() / "Assets" / "Shaders";
            if (std::filesystem::exists(shaderPath)) {
                m_watcher->watch(shaderPath, [this](const AssetChange &change) {
                    if (change.path.extension() == ".vert" ||
                        change.path.extension() == ".frag" ||
                        change.path.extension() == ".glsl") {
                        LOG_INFO("Shader asset changed: {}", change.path.string());
                        for (auto &callback: m_shaderCallbacks) {
                            callback(change.path);
                        }
                    }
                });
            }
        }

        m_initialized = true;
        LOG_INFO("AssetManager initialized successfully");
    }

    void AssetManager::Shutdown() {
        if (!m_initialized) {
            return;
        }

        LOG_INFO("Shutting down AssetManager...");

        ClearCache();
        m_watcher.reset();

        m_meshCallbacks.clear();
        m_shaderCallbacks.clear();
        m_scriptCallbacks.clear();

        m_initialized = false;
        LOG_INFO("AssetManager shut down");
    }

    void AssetManager::Update() {
        if (m_watcher) {
            m_watcher->pump();
        }
    }

    // ============================================================================
    // Mesh Management
    // ============================================================================

    MeshHandle AssetManager::LoadMesh(const std::string &relativePath) {
        if (relativePath.empty()) {
            LOG_ERROR("AssetManager::LoadMesh: Empty path");
            return MeshHandle();
        }

        // Normalize path
        auto normalizedPath = ResolvePath(relativePath);
        std::string pathKey = normalizedPath.string();

        // Check if already loaded
        if (auto it = m_meshPathToID.find(pathKey); it != m_meshPathToID.end()) {
            LOG_DEBUG("Mesh already cached: {}", relativePath);
            return MeshHandle(it->second);
        }

        // Load from file
        auto mesh = LoadMeshFromFile(normalizedPath);
        if (!mesh) {
            LOG_ERROR("Failed to load mesh: {}", relativePath);
            return MeshHandle();
        }

        // Store in cache
        MeshHandle::IDType id = m_nextMeshID++;
        MeshHandle handle(id);

        uint64_t lastModified = 0;
        if (std::filesystem::exists(normalizedPath)) {
            auto ftime = std::filesystem::last_write_time(normalizedPath);
            lastModified = std::chrono::duration_cast<std::chrono::seconds>(
                ftime.time_since_epoch()
            ).count();
        }

        m_meshes[id] = MeshEntry{
            std::move(mesh),
            normalizedPath,
            lastModified
        };
        m_meshPathToID[pathKey] = id;

        LOG_INFO("Loaded mesh: {} (ID: {}, Vertices: {}, Indices: {})",
                 relativePath, id,
                 m_meshes[id].mesh->vertices.size(),
                 m_meshes[id].mesh->indices.size());

        return handle;
    }

    const Mesh *AssetManager::GetMesh(MeshHandle handle) const {
        if (!handle.IsValid()) {
            return nullptr;
        }

        if (auto it = m_meshes.find(handle.ID()); it != m_meshes.end()) {
            return it->second.mesh.get();
        }

        return nullptr;
    }

    void AssetManager::UnloadMesh(MeshHandle handle) {
        if (!handle.IsValid()) {
            return;
        }

        if (auto it = m_meshes.find(handle.ID()); it != m_meshes.end()) {
            // Remove from path map
            std::string pathKey = it->second.path.string();
            m_meshPathToID.erase(pathKey);

            // Remove from mesh map
            LOG_DEBUG("Unloaded mesh: {}", pathKey);
            m_meshes.erase(it);
        }
    }

    // ============================================================================
    // OBJ Parsing (from MeshManager)
    // ============================================================================

    std::unique_ptr<Mesh> AssetManager::LoadMeshFromFile(const std::filesystem::path &path) {
        if (!std::filesystem::exists(path)) {
            LOG_ERROR("Mesh file does not exist: {}", path.string());
            return nullptr;
        }

        auto ext = path.extension().string();
        if (ext == ".obj") {
            auto mesh = std::make_unique<Mesh>(ParseOBJ(path));
            return mesh;
        }

        LOG_ERROR("Unsupported mesh format: {}", ext);
        return nullptr;
    }

    Mesh AssetManager::ParseOBJ(const std::filesystem::path &path) {
        Mesh mesh;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoords;

        std::ifstream file(path);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open OBJ: {}", path.string());
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
                // Collect all vertices in this face (supports triangles, quads, n-gons)
                std::vector<MeshVertex> faceVertices;
                std::string vertSpec;

                while (iss >> vertSpec) {
                    int v = 0, vt = 0, vn = 0;

                    // Parse vertex specification (supports v, v/vt, v/vt/vn, v//vn formats)
                    if (sscanf_s(vertSpec.c_str(), "%d/%d/%d", &v, &vt, &vn) == 3) {
                        // v/vt/vn
                    } else if (sscanf_s(vertSpec.c_str(), "%d//%d", &v, &vn) == 2) {
                        // v//vn
                        vt = 0;
                    } else if (sscanf_s(vertSpec.c_str(), "%d/%d", &v, &vt) == 2) {
                        // v/vt
                        vn = 0;
                    } else if (sscanf_s(vertSpec.c_str(), "%d", &v) == 1) {
                        // v only
                        vt = 0;
                        vn = 0;
                    }

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

                    faceVertices.push_back(vertex);
                }

                // Triangulate the face using fan triangulation
                if (faceVertices.size() >= 3) {
                    const uint32_t baseIndex = static_cast<uint32_t>(mesh.vertices.size());

                    // Add all vertices
                    for (const auto& vertex : faceVertices) {
                        mesh.vertices.push_back(vertex);
                    }

                    // Create triangles using fan triangulation
                    for (size_t i = 1; i + 1 < faceVertices.size(); ++i) {
                        mesh.indices.push_back(baseIndex);         // First vertex
                        mesh.indices.push_back(baseIndex + i);     // Current vertex
                        mesh.indices.push_back(baseIndex + i + 1); // Next vertex
                    }
                }
            }
        }

        if (mesh.vertices.empty()) {
            LOG_WARN("OBJ has no vertices: {}", path.string());
        }

        return mesh;
    }

    // ============================================================================
    // Asset Callbacks
    // ============================================================================

    void AssetManager::OnMeshChanged(AssetChangeCallback callback) {
        if (callback) {
            m_meshCallbacks.push_back(std::move(callback));
        }
    }

    void AssetManager::OnShaderChanged(AssetChangeCallback callback) {
        if (callback) {
            m_shaderCallbacks.push_back(std::move(callback));
        }
    }

    void AssetManager::OnScriptChanged(AssetChangeCallback callback) {
        if (callback) {
            m_scriptCallbacks.push_back(std::move(callback));
        }
    }

    // ============================================================================
    // Path Management
    // ============================================================================

    std::filesystem::path AssetManager::ResolvePath(const std::string &relativePath) const {
        return Core::Paths::ResolveAssetPath(relativePath);
    }

    bool AssetManager::FileExists(const std::string &relativePath) const {
        return std::filesystem::exists(ResolvePath(relativePath));
    }

    // ============================================================================
    // Statistics & Maintenance
    // ============================================================================

    size_t AssetManager::GetMemoryUsage() const {
        size_t total = 0;

        for (const auto &[id, entry]: m_meshes) {
            if (entry.mesh) {
                total += entry.mesh->vertices.size() * sizeof(MeshVertex);
                total += entry.mesh->indices.size() * sizeof(uint32_t);
            }
        }

        return total;
    }

    void AssetManager::ClearCache() {
        m_meshes.clear();
        m_meshPathToID.clear();
        m_nextMeshID = 1;

        LOG_INFO("Asset cache cleared");
    }
} // namespace Artifact::Assets
