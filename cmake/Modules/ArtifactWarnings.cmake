function(artifact_enable_warnings)
    if(TARGET artifact_warnings)
        return()
    endif()

    add_library(artifact_warnings INTERFACE)

    target_compile_features(artifact_warnings INTERFACE cxx_std_23)

    target_compile_options(artifact_warnings INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive- /EHsc /w44265 /w44263>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion>
    )

    target_compile_definitions(artifact_warnings INTERFACE
        $<$<CONFIG:Debug>:ARTIFACT_DEBUG=1>
    )
endfunction()

function(artifact_target_enable_warnings target)
    if(TARGET artifact_warnings)
        target_link_libraries(${target} PRIVATE artifact_warnings)
    endif()
endfunction()
