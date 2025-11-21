cmake_minimum_required(VERSION 3.24)

project(Artifact VERSION 0.1.0 LANGUAGES CXX C)

# Ensure MSVC linker matches the compiler toolset (avoid mixing 14.50 object code with 14.44 linker/libs)
if (MSVC AND CMAKE_CXX_COMPILER)
    get_filename_component(_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    if (EXISTS "${_cl_dir}/link.exe")
        # Force linker to same toolset version as compiler
        set(CMAKE_LINKER "${_cl_dir}/link.exe" CACHE FILEPATH "MSVC linker" FORCE)
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

include(ArtifactOptions)
include(ArtifactWarnings)
include(ArtifactDependencies)

artifact_import_dependencies()
artifact_configure_options()
artifact_enable_warnings()

add_subdirectory(EngineCore)

if(ARTIFACT_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ARTIFACT_ENABLE_EDITOR)
    add_subdirectory(Editor)
endif()

if(ARTIFACT_ENABLE_RUNTIME)
    add_subdirectory(Runtime)
endif()

add_subdirectory(Plugins)
add_subdirectory(Tools)
add_subdirectory(Games)

if(ARTIFACT_BUILD_PACKAGING)
    add_subdirectory(Packaging)
endif()
