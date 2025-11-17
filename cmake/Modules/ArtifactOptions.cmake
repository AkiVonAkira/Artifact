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
