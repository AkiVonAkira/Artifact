cmake_minimum_required(VERSION 3.24)

project(Artifact VERSION 0.1.0 LANGUAGES CXX C)

# =============================================================================
# Platform detection and compiler setup
# =============================================================================

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Ensure MSVC linker matches the compiler toolset (avoid mixing 14.50 object code with 14.44 linker/libs)
if(MSVC AND CMAKE_CXX_COMPILER)
    get_filename_component(_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    if(EXISTS "${_cl_dir}/link.exe")
        # Force linker to same toolset version as compiler
        set(CMAKE_LINKER "${_cl_dir}/link.exe" CACHE FILEPATH "MSVC linker" FORCE)
    endif()
endif()

# =============================================================================
# Module path setup
# =============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# =============================================================================
# Include configuration modules
# =============================================================================

include(ArtifactOptions)
include(ArtifactWarnings)
include(ArtifactDependencies)

artifact_configure_options()
artifact_import_dependencies()
artifact_enable_warnings()

# =============================================================================
# Add subdirectories
# =============================================================================

# Core engine (required)
add_subdirectory(EngineCore)

# Plugins (optional, only if directory exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Plugins/CMakeLists.txt")
    add_subdirectory(Plugins)
endif()

# Editor (optional)
if(ARTIFACT_ENABLE_EDITOR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Editor/CMakeLists.txt")
    add_subdirectory(Editor)
endif()

# Runtime (optional)
if(ARTIFACT_ENABLE_RUNTIME AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Runtime/CMakeLists.txt")
    add_subdirectory(Runtime)
endif()

# Tests (optional)
if(ARTIFACT_ENABLE_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()

# Tools (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Tools/CMakeLists.txt")
    add_subdirectory(Tools)
endif()

# Games (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Games/CMakeLists.txt")
    add_subdirectory(Games)
endif()

# Packaging (optional)
if(ARTIFACT_BUILD_PACKAGING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Packaging/CMakeLists.txt")
    add_subdirectory(Packaging)
endif()

# =============================================================================
# Status summary
# =============================================================================

message(STATUS "=================================================================")
message(STATUS "Artifact Configuration Summary")
message(STATUS "=================================================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  ARTIFACT_ENABLE_EDITOR:   ${ARTIFACT_ENABLE_EDITOR}")
message(STATUS "  ARTIFACT_ENABLE_RUNTIME:  ${ARTIFACT_ENABLE_RUNTIME}")
message(STATUS "  ARTIFACT_ENABLE_TESTS:    ${ARTIFACT_ENABLE_TESTS}")
message(STATUS "  ARTIFACT_ENABLE_TRACY:    ${ARTIFACT_ENABLE_TRACY}")
message(STATUS "  ARTIFACT_ENABLE_TOOLS:    ${ARTIFACT_ENABLE_TOOLS}")
message(STATUS "  ARTIFACT_BUILD_PACKAGING: ${ARTIFACT_BUILD_PACKAGING}")
message(STATUS "=================================================================")
