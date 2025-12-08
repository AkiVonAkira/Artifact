cmake_minimum_required(VERSION 3.24)

project(Artifact VERSION 0.1.0 LANGUAGES CXX C)

# =============================================================================
# Build type setup (for single-config generators like Ninja/Makefiles)
# =============================================================================
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as none was specified")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# =============================================================================
# CMake policies for consistent behavior
# =============================================================================
cmake_policy(SET CMP0077 NEW)  # option() honors normal variables

# =============================================================================
# Module path
# =============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# =============================================================================
# Project options (must come before dependencies)
# =============================================================================
include(ArtifactOptions)
artifact_configure_options()

# =============================================================================
# Compiler requirements and warnings
# =============================================================================
include(ArtifactWarnings)
artifact_enable_warnings()

# =============================================================================
# Third-party dependencies (vcpkg or system packages)
# =============================================================================
include(ArtifactDependencies)
artifact_import_dependencies()

# =============================================================================
# Subdirectories
# =============================================================================

# Core engine library (always required)
add_subdirectory(EngineCore)

# Editor executable (optional)
if(ARTIFACT_ENABLE_EDITOR)
    add_subdirectory(Editor)
endif()

# Runtime executable (optional)
if(ARTIFACT_ENABLE_RUNTIME)
    add_subdirectory(Runtime)
endif()

# Tests (optional)
if(ARTIFACT_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Tools (optional)
if(ARTIFACT_ENABLE_TOOLS)
    add_subdirectory(Tools)
endif()

# Plugins (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Plugins/CMakeLists.txt")
    add_subdirectory(Plugins)
endif()

# Games (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Games/CMakeLists.txt")
    add_subdirectory(Games)
endif()

# Packaging (optional)
if(ARTIFACT_BUILD_PACKAGING)
    add_subdirectory(Packaging)
endif()

# =============================================================================
# Configuration summary
# =============================================================================
message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "  Artifact Engine Configuration")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "  Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard:   ${CMAKE_CXX_STANDARD}")
message(STATUS "  Platform:       ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler:       ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "  Components:")
message(STATUS "    Editor:       ${ARTIFACT_ENABLE_EDITOR}")
message(STATUS "    Runtime:      ${ARTIFACT_ENABLE_RUNTIME}")
message(STATUS "    Tests:        ${ARTIFACT_ENABLE_TESTS}")
message(STATUS "    Tools:        ${ARTIFACT_ENABLE_TOOLS}")
message(STATUS "    Packaging:    ${ARTIFACT_BUILD_PACKAGING}")
message(STATUS "")
message(STATUS "  Features:")
message(STATUS "    Tracy:        ${ARTIFACT_ENABLE_TRACY}")
message(STATUS "")
message(STATUS "  Output dir:     ${CMAKE_BINARY_DIR}/bin")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "")
