# Runtime Executable

cmake_minimum_required(VERSION 3.21)

# Check if main.cpp exists
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
    message(WARNING "Runtime/main.cpp not found - skipping runtime executable")
    return()
endif()

# =============================================================================
# Executable target
# =============================================================================

add_executable(artifact_runtime main.cpp)

# =============================================================================
# Link dependencies
# =============================================================================

target_link_libraries(artifact_runtime 
    PRIVATE 
        Artifact::EngineCore
        artifact_third_party
)

# =============================================================================
# Compile definitions
# =============================================================================

target_compile_definitions(artifact_runtime PRIVATE ARTIFACT_IS_RUNTIME=1)

# =============================================================================
# Compiler warnings
# =============================================================================

artifact_target_enable_warnings(artifact_runtime)

# =============================================================================
# IDE support
# =============================================================================

set_target_properties(artifact_runtime PROPERTIES 
    FOLDER "Runtime"
    OUTPUT_NAME "ArtifactRuntime"
)

# =============================================================================
# Output directory (cross-platform)
# =============================================================================

if(WIN32)
    set(ARTIFACT_PLATFORM_DIR "Windows")
elseif(APPLE)
    set(ARTIFACT_PLATFORM_DIR "macOS")
elseif(UNIX)
    set(ARTIFACT_PLATFORM_DIR "Linux")
else()
    set(ARTIFACT_PLATFORM_DIR "Unknown")
endif()

set_target_properties(artifact_runtime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${ARTIFACT_PLATFORM_DIR}/${CMAKE_BUILD_TYPE}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/${ARTIFACT_PLATFORM_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/${ARTIFACT_PLATFORM_DIR}/Release"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin/${ARTIFACT_PLATFORM_DIR}/RelWithDebInfo"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin/${ARTIFACT_PLATFORM_DIR}/MinSizeRel"
)

# =============================================================================
# Copy runtime dependencies
# =============================================================================

if(WIN32)
    add_custom_command(TARGET artifact_runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:artifact_runtime>
            $<TARGET_FILE_DIR:artifact_runtime>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime dependencies..."
    )
endif()

# =============================================================================
# Installation rules
# =============================================================================

install(TARGETS artifact_runtime
    RUNTIME DESTINATION bin
)
