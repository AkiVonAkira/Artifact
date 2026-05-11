# EngineCore Library - Modern CMake with Auto-Discovery

cmake_minimum_required(VERSION 3.21)

# =============================================================================
# Auto-discover source files
# =============================================================================

# Public headers (API that users of the library can include)
file(GLOB_RECURSE ENGINECORE_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Public/Artifact/*.h"
)

# Private headers (internal implementation details)
file(GLOB_RECURSE ENGINECORE_PRIVATE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Private/Artifact/*.h"
)

# Source files (all .cpp files)
file(GLOB_RECURSE ENGINECORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

# Print discovered files for debugging
list(LENGTH ENGINECORE_PUBLIC_HEADERS PUBLIC_COUNT)
list(LENGTH ENGINECORE_PRIVATE_HEADERS PRIVATE_COUNT)
list(LENGTH ENGINECORE_SOURCES SOURCE_COUNT)

message(STATUS "EngineCore: Found ${PUBLIC_COUNT} public headers")
message(STATUS "EngineCore: Found ${PRIVATE_COUNT} private headers")
message(STATUS "EngineCore: Found ${SOURCE_COUNT} source files")

# Ensure we found files
if(SOURCE_COUNT EQUAL 0)
    message(FATAL_ERROR "No source files found in ${CMAKE_CURRENT_SOURCE_DIR}/Source")
endif()

# =============================================================================
# Library target
# =============================================================================

add_library(artifact_engine STATIC
    ${ENGINECORE_PUBLIC_HEADERS}
    ${ENGINECORE_PRIVATE_HEADERS}
    ${ENGINECORE_SOURCES}
)

# Alias for modern CMake
add_library(Artifact::EngineCore ALIAS artifact_engine)

# =============================================================================
# Include directories
# =============================================================================

target_include_directories(artifact_engine
    PUBLIC
        # Public API - available to consumers
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        # Private headers - only for this library
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# =============================================================================
# Compile definitions
# =============================================================================

target_compile_definitions(artifact_engine
    PUBLIC
        ARTIFACT_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

# =============================================================================
# Link dependencies
# =============================================================================

target_link_libraries(artifact_engine
    PUBLIC
        artifact_third_party
        glm::glm
    PRIVATE
        # ...existing code...
)

# =============================================================================
# Compiler warnings
# =============================================================================

artifact_target_enable_warnings(artifact_engine)

# =============================================================================
# Tracy profiler support
# =============================================================================

if(ARTIFACT_ENABLE_TRACY)
    target_compile_definitions(artifact_engine PUBLIC TRACY_ENABLE=1)
endif()

# =============================================================================
# IDE support - organize files in folders
# =============================================================================

# Group headers by directory
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PREFIX "Public"
    FILES ${ENGINECORE_PUBLIC_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private
    PREFIX "Private"
    FILES ${ENGINECORE_PRIVATE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source
    PREFIX "Source"
    FILES ${ENGINECORE_SOURCES}
)

# =============================================================================
# Installation rules (optional)
# =============================================================================

install(TARGETS artifact_engine
    EXPORT ArtifactEngineTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY Public/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
