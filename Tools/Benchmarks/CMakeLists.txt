# Simple timing benchmark for mesh loading
cmake_minimum_required(VERSION 3.21)

project(ArtifactMeshLoadBenchmark LANGUAGES CXX)

add_executable(artifact_mesh_benchmark MeshLoadBenchmark.cpp)

# Link against EngineCore (for MeshManager, Paths, logging)
target_link_libraries(artifact_mesh_benchmark PRIVATE Artifact::EngineCore)

# Ensure glm is available (EngineCore links it privately; add header-only for this tool)
find_package(glm CONFIG REQUIRED)
target_link_libraries(artifact_mesh_benchmark PRIVATE glm::glm-header-only)

# Add vcpkg include path (fallback)
target_include_directories(artifact_mesh_benchmark PRIVATE
    ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/include
)

# Output to bin like other targets
set_target_properties(artifact_mesh_benchmark PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
