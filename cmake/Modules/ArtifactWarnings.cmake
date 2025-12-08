# =============================================================================
# Artifact Compiler Warnings
# =============================================================================

function(artifact_enable_warnings)
    if(TARGET artifact_warnings)
        return()
    endif()

    add_library(artifact_warnings INTERFACE)

    # Require C++23
    target_compile_features(artifact_warnings INTERFACE cxx_std_23)

    # =============================================================================
    # MSVC Warnings
    # =============================================================================
    target_compile_options(artifact_warnings INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4                    # High warning level
            /permissive-          # Standards conformance
            /EHsc                 # Exception handling model
            /w44265               # Class has virtual functions, but destructor is not virtual
            /w44263               # Member function does not override any base class virtual member function
            /w44062               # Enumerator in switch is not handled
            /wd4100               # Unreferenced formal parameter (too noisy)
        >
    )

    # =============================================================================
    # GCC/Clang Warnings
    # =============================================================================
    target_compile_options(artifact_warnings INTERFACE
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
            -Wall                  # Basic warnings
            -Wextra                # Extra warnings
            -Wpedantic             # ISO C++ compliance
            -Wconversion           # Implicit type conversions
            -Wsign-conversion      # Sign conversions
            -Wcast-align           # Pointer cast alignment
            -Wunused               # Unused variables/functions
            -Wno-unused-parameter  # Allow unused parameters (callbacks, interfaces)
            -Wnon-virtual-dtor     # Non-virtual destructor
            -Woverloaded-virtual   # Overloaded virtual function
        >
    )

    # =============================================================================
    # Debug/Release definitions
    # =============================================================================
    target_compile_definitions(artifact_warnings INTERFACE
        $<$<CONFIG:Debug>:
            ARTIFACT_DEBUG=1
            ARTIFACT_ENABLE_ASSERTS=1
        >
        $<$<CONFIG:Release>:
            ARTIFACT_RELEASE=1
            NDEBUG
        >
        $<$<CONFIG:RelWithDebInfo>:
            ARTIFACT_RELEASE=1
            ARTIFACT_DEBUG_INFO=1
        >
    )
endfunction()

# Helper function to apply warnings to a target
function(artifact_target_enable_warnings target)
    if(NOT TARGET artifact_warnings)
        message(WARNING "artifact_warnings target not found. Call artifact_enable_warnings() first.")
        return()
    endif()

    if(NOT TARGET ${target})
        message(FATAL_ERROR "Target '${target}' does not exist")
    endif()

    target_link_libraries(${target} PRIVATE artifact_warnings)
endfunction()
