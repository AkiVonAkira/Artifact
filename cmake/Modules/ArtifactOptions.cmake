# =============================================================================
# Artifact Build Options
# =============================================================================

function(artifact_configure_options)
    # Component toggles
    option(ARTIFACT_ENABLE_EDITOR "Build the Artifact editor" ON)
    option(ARTIFACT_ENABLE_RUNTIME "Build the standalone runtime" OFF)
    option(ARTIFACT_ENABLE_TESTS "Build Artifact tests" ON)
    option(ARTIFACT_BUILD_PACKAGING "Enable packaging utilities" OFF)
    option(ARTIFACT_ENABLE_TRACY "Enable Tracy profiling" ON)
    option(ARTIFACT_ENABLE_TOOLS "Build developer tools" OFF)

    # Advanced options
    option(ARTIFACT_USE_UNITY_BUILD "Use unity builds for faster compilation" OFF)

    # =============================================================================
    # C++ Standard
    # =============================================================================
    set(CMAKE_CXX_STANDARD 23 CACHE STRING "C++ standard" FORCE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "Require C++ standard" FORCE)
    set(CMAKE_CXX_EXTENSIONS OFF CACHE BOOL "Disable compiler-specific extensions" FORCE)

    # =============================================================================
    # Compiler version checks
    # =============================================================================
    if(MSVC)
        if(MSVC_VERSION LESS 1930)
            message(FATAL_ERROR "Artifact requires Visual Studio 2022 (MSVC 19.30) or newer")
        endif()

        # Ensure MSVC linker matches compiler toolset
        if(CMAKE_CXX_COMPILER)
            get_filename_component(_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
            if(EXISTS "${_cl_dir}/link.exe")
                set(CMAKE_LINKER "${_cl_dir}/link.exe" CACHE FILEPATH "MSVC linker" FORCE)
            endif()
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "11.0")
            message(FATAL_ERROR "Artifact requires GCC 11.0 or newer")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
            message(FATAL_ERROR "Artifact requires Clang 13.0 or newer")
        endif()
    endif()

    # =============================================================================
    # Platform detection
    # =============================================================================
    set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Enable PIC" FORCE)

    if(WIN32)
        set(ARTIFACT_PLATFORM "Windows" CACHE STRING "Target platform" FORCE)
    elseif(APPLE)
        set(ARTIFACT_PLATFORM "macOS" CACHE STRING "Target platform" FORCE)
    elseif(UNIX)
        set(ARTIFACT_PLATFORM "Linux" CACHE STRING "Target platform" FORCE)
    else()
        set(ARTIFACT_PLATFORM "Unknown" CACHE STRING "Target platform" FORCE)
        message(WARNING "Building on unvalidated platform: ${CMAKE_SYSTEM_NAME}")
    endif()

    # =============================================================================
    # Link-Time Optimization (LTO/IPO) for Release builds
    # =============================================================================
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_output LANGUAGES CXX)

    if(_ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON CACHE BOOL "" FORCE)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON CACHE BOOL "" FORCE)
        message(STATUS "IPO/LTO enabled for Release builds")
    else()
        message(STATUS "IPO/LTO not supported: ${_ipo_output}")
    endif()

    # =============================================================================
    # Compiler-specific global flags
    # =============================================================================
    if(MSVC)
        # Parallel compilation
        add_compile_options(/MP)
        # UTF-8 source and execution character sets
        add_compile_options(/utf-8)
        # Disable some overly pedantic warnings
        add_compile_options(/wd4251)  # DLL interface warnings
    else()
        # Colored diagnostics
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            add_compile_options(-fdiagnostics-color=always)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-fcolor-diagnostics)
        endif()
    endif()

    # =============================================================================
    # IDE support
    # =============================================================================
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

    # Export compile commands for tools like clangd, clang-tidy
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

    message(STATUS "Artifact configured for ${ARTIFACT_PLATFORM}")
endfunction()
