function(artifact_configure_options)
    option(ARTIFACT_ENABLE_EDITOR "Build the Artifact editor" ON)
    option(ARTIFACT_ENABLE_RUNTIME "Build the standalone runtime" ON)
    option(ARTIFACT_ENABLE_TESTS "Build Artifact tests" ON)
    option(ARTIFACT_BUILD_PACKAGING "Enable packaging utilities" ON)
    option(ARTIFACT_ENABLE_TRACY "Enable Tracy profiling" ON)
    option(ARTIFACT_ENABLE_TOOLS "Build developer tools" ON)

    set(CMAKE_CXX_STANDARD 23 CACHE STRING "" FORCE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    if(NOT DEFINED ARTIFACT_HOST_PLATFORM)
        string(TOLOWER "${CMAKE_SYSTEM_NAME}" _artifact_host_lower)
        if(_artifact_host_lower STREQUAL "darwin")
            set(_artifact_host "macos")
        elseif(_artifact_host_lower STREQUAL "windows")
            set(_artifact_host "windows")
        elseif(_artifact_host_lower STREQUAL "linux")
            set(_artifact_host "linux")
        else()
            set(_artifact_host "${_artifact_host_lower}")
            message(WARNING
                "Artifact is being configured on an unvalidated platform (${CMAKE_SYSTEM_NAME})."
            )
        endif()
        set(ARTIFACT_HOST_PLATFORM "${_artifact_host}" CACHE STRING "Normalized host platform used by presets/scripts" FORCE)
    endif()

    set(_artifact_host_is_windows OFF)
    set(_artifact_host_is_linux OFF)
    set(_artifact_host_is_macos OFF)
    if(ARTIFACT_HOST_PLATFORM STREQUAL "windows")
        set(_artifact_host_is_windows ON)
    elseif(ARTIFACT_HOST_PLATFORM STREQUAL "linux")
        set(_artifact_host_is_linux ON)
    elseif(ARTIFACT_HOST_PLATFORM STREQUAL "macos")
        set(_artifact_host_is_macos ON)
    endif()

    set(ARTIFACT_HOST_IS_WINDOWS ${_artifact_host_is_windows} CACHE BOOL "True if Artifact is configuring on Windows" FORCE)
    set(ARTIFACT_HOST_IS_LINUX ${_artifact_host_is_linux} CACHE BOOL "True if Artifact is configuring on Linux" FORCE)
    set(ARTIFACT_HOST_IS_MACOS ${_artifact_host_is_macos} CACHE BOOL "True if Artifact is configuring on macOS" FORCE)

    message(STATUS "Artifact host platform detected: ${ARTIFACT_HOST_PLATFORM}")

    if(NOT DEFINED ARTIFACT_THIRD_PARTY_ROOT)
        set(ARTIFACT_THIRD_PARTY_ROOT "${CMAKE_SOURCE_DIR}/ThirdParty" CACHE PATH "Root directory where shared third-party builds will be staged" FORCE)
    endif()

    if(DEFINED CMAKE_PRESET_NAME)
        set(_artifact_third_party_key "${CMAKE_PRESET_NAME}")
    elseif(CMAKE_CONFIGURATION_TYPES)
        set(_artifact_third_party_key "multi-config")
    elseif(CMAKE_BUILD_TYPE)
        string(TOLOWER "${CMAKE_BUILD_TYPE}" _artifact_third_party_key)
    else()
        set(_artifact_third_party_key "default")
    endif()

    set(ARTIFACT_THIRD_PARTY_DIR "${ARTIFACT_THIRD_PARTY_ROOT}/${ARTIFACT_HOST_PLATFORM}/${_artifact_third_party_key}"
        CACHE PATH "Variant-specific folder where FetchContent dependencies are cached" FORCE)
    file(MAKE_DIRECTORY "${ARTIFACT_THIRD_PARTY_DIR}")
    set(FETCHCONTENT_BASE_DIR "${ARTIFACT_THIRD_PARTY_DIR}" CACHE PATH "Root used for FetchContent dependency trees" FORCE)

    message(STATUS "Artifact third-party cache: ${FETCHCONTENT_BASE_DIR}")

    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_msg)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    endif()

    if(MSVC)
        add_compile_options(/MP)
    else()
        add_compile_options(-fdiagnostics-color=always)
    endif()

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endfunction()
