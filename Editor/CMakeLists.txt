# Editor Executable - Modern CMake with Auto-Discovery

cmake_minimum_required(VERSION 3.21)

# =============================================================================
# Auto-discover source files
# =============================================================================

# Editor public headers (if plugins need them)
file(GLOB_RECURSE EDITOR_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Public/Artifact/Editor/*.h"
)

# All editor source files
file(GLOB_RECURSE EDITOR_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"  # Include local headers in subfolders
)

# Print discovered files
list(LENGTH EDITOR_PUBLIC_HEADERS PUBLIC_COUNT)
list(LENGTH EDITOR_SOURCES SOURCE_COUNT)

message(STATUS "Editor: Found ${PUBLIC_COUNT} public headers")
message(STATUS "Editor: Found ${SOURCE_COUNT} source files")

# Ensure we found files
if(SOURCE_COUNT EQUAL 0)
    message(FATAL_ERROR "No source files found in ${CMAKE_CURRENT_SOURCE_DIR}/Source")
endif()

# =============================================================================
# Executable target
# =============================================================================

add_executable(ArtifactEditor
    ${EDITOR_PUBLIC_HEADERS}
    ${EDITOR_SOURCES}
)

# =============================================================================
# Include directories
# =============================================================================

target_include_directories(ArtifactEditor
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# =============================================================================
# Link dependencies
# =============================================================================

target_link_libraries(ArtifactEditor
    PRIVATE
        Artifact::EngineCore
        artifact_third_party
)

# =============================================================================
# Compiler warnings
# =============================================================================

artifact_target_enable_warnings(ArtifactEditor)

# =============================================================================
# IDE support - organize files in folders
# =============================================================================

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PREFIX "Public"
    FILES ${EDITOR_PUBLIC_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source
    PREFIX "Source"
    FILES ${EDITOR_SOURCES}
)

# =============================================================================
# Output directory
# =============================================================================

set_target_properties(ArtifactEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Windows/${CMAKE_BUILD_TYPE}"
    OUTPUT_NAME "ArtifactEditor"
)

# =============================================================================
# Copy runtime dependencies (if needed)
# =============================================================================

# Copy DLLs on Windows
if(WIN32)
    add_custom_command(TARGET ArtifactEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:ArtifactEditor>
        $<TARGET_FILE_DIR:ArtifactEditor>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime dependencies..."
    )
endif()

