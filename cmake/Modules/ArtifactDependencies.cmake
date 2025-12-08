
# =============================================================================
# Helper: Pick the first available target from a list
# =============================================================================
function(_artifact_pick_target output_var)
    foreach(candidate ${ARGN})
        if(TARGET ${candidate})
            set(${output_var} ${candidate} PARENT_SCOPE)
            return()
        endif()
    endforeach()

    list(JOIN ARGN ", " _candidates_str)
    message(FATAL_ERROR
        "None of the expected targets exist: [${_candidates_str}]\n"
        "This usually means the package was not found or installed correctly.\n"
        "If using vcpkg, ensure 'vcpkg.json' lists this dependency and re-run CMake."
    )
endfunction()

# =============================================================================
# Main dependency import function
# =============================================================================
function(artifact_import_dependencies)
    if(TARGET artifact_third_party)
        return()
    endif()

    message(STATUS "")
    message(STATUS "═══════════════════════════════════════════════════════════")
    message(STATUS "  Importing Artifact Dependencies")
    message(STATUS "═══════════════════════════════════════════════════════════")

    # GLFW - Window and input
    find_package(glfw3 CONFIG REQUIRED)
    _artifact_pick_target(_artifact_glfw_target glfw glfw3::glfw)
    message(STATUS "  ✓ GLFW:          ${_artifact_glfw_target}")

    # GLAD - OpenGL loader
    find_package(glad CONFIG REQUIRED)
    _artifact_pick_target(_artifact_glad_target glad::glad glad)
    message(STATUS "  ✓ GLAD:          ${_artifact_glad_target}")

    # ImGui - UI framework
    find_package(imgui CONFIG REQUIRED)
    _artifact_pick_target(_artifact_imgui_target imgui::imgui imgui)
    message(STATUS "  ✓ ImGui:         ${_artifact_imgui_target}")

    # ImGuizmo - Gizmo controls
    find_package(imguizmo CONFIG REQUIRED)
    _artifact_pick_target(_artifact_imguizmo_target ImGuizmo::ImGuizmo ImGuizmo imguizmo::imguizmo)
    message(STATUS "  ✓ ImGuizmo:      ${_artifact_imguizmo_target}")

    # ImGuiFileDialog - File browser (optional, fallback to FetchContent)
    find_package(imgui-file-dialog CONFIG QUIET)
    if(imgui-file-dialog_FOUND)
        _artifact_pick_target(_artifact_imgui_file_dialog_target ImGuiFileDialog::ImGuiFileDialog ImGuiFileDialog)
        message(STATUS "  ✓ ImGuiFileDialog: ${_artifact_imgui_file_dialog_target}")
    else()
        message(STATUS "  ⚠ ImGuiFileDialog not in vcpkg, fetching from GitHub...")
        include(FetchContent)
        FetchContent_Declare(imgui_file_dialog
            GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
            GIT_TAG v0.6.8
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(imgui_file_dialog)

        add_library(artifact_imgui_file_dialog STATIC
            ${imgui_file_dialog_SOURCE_DIR}/ImGuiFileDialog.cpp
            ${imgui_file_dialog_SOURCE_DIR}/ImGuiFileDialog.h
        )
        target_include_directories(artifact_imgui_file_dialog PUBLIC ${imgui_file_dialog_SOURCE_DIR})
        target_link_libraries(artifact_imgui_file_dialog PRIVATE ${_artifact_imgui_target})
        set(_artifact_imgui_file_dialog_target artifact_imgui_file_dialog)
        message(STATUS "  ✓ ImGuiFileDialog: Built from source")
    endif()

    # spdlog - Logging
    find_package(spdlog CONFIG REQUIRED)
    _artifact_pick_target(_artifact_spdlog_target spdlog::spdlog_header_only spdlog::spdlog spdlog)
    message(STATUS "  ✓ spdlog:        ${_artifact_spdlog_target}")

    # GLM - Math library
    find_package(glm CONFIG REQUIRED)
    _artifact_pick_target(_artifact_glm_target glm::glm glm)
    message(STATUS "  ✓ GLM:           ${_artifact_glm_target}")

    # EnTT - Entity component system
    find_package(entt CONFIG REQUIRED)
    _artifact_pick_target(_artifact_entt_target EnTT::EnTT entt)
    message(STATUS "  ✓ EnTT:          ${_artifact_entt_target}")

    # EFSW - File watcher
    find_package(efsw CONFIG REQUIRED)
    _artifact_pick_target(_artifact_efsw_target efsw::efsw efsw)
    message(STATUS "  ✓ EFSW:          ${_artifact_efsw_target}")

    # Tracy - Profiler
    find_package(tracy CONFIG REQUIRED)
    _artifact_pick_target(_artifact_tracy_target Tracy::TracyClient TracyClient)
    message(STATUS "  ✓ Tracy:         ${_artifact_tracy_target}")

    # Box2D - 2D physics
    find_package(box2d CONFIG REQUIRED)
    _artifact_pick_target(_artifact_box2d_target box2d::box2d Box2D::Box2D box2d)
    message(STATUS "  ✓ Box2D:         ${_artifact_box2d_target}")

    # Jolt - 3D physics
    find_package(jolt CONFIG QUIET)
    if(jolt_FOUND OR TARGET Jolt::Jolt OR TARGET jolt)
        _artifact_pick_target(_artifact_jolt_target Jolt::Jolt jolt)
        message(STATUS "  ✓ Jolt:          ${_artifact_jolt_target}")
    else()
        find_package(JoltPhysics CONFIG QUIET)
        if(JoltPhysics_FOUND)
            _artifact_pick_target(_artifact_jolt_target Jolt::Jolt JoltPhysics::JoltPhysics)
            message(STATUS "  ✓ JoltPhysics:   ${_artifact_jolt_target}")
        else()
            message(FATAL_ERROR "Jolt Physics not found. Install via vcpkg: vcpkg install joltphysics")
        endif()
    endif()

    # Assimp - Asset importer
    find_package(assimp CONFIG REQUIRED)
    _artifact_pick_target(_artifact_assimp_target assimp::assimp assimp)
    message(STATUS "  ✓ Assimp:        ${_artifact_assimp_target}")

    # STB - Image loading (header-only)
    find_package(Stb REQUIRED)
    if(TARGET Stb::stb OR TARGET Stb::Stb)
        _artifact_pick_target(_artifact_stb_target Stb::stb Stb::Stb)
        message(STATUS "  ✓ STB:           ${_artifact_stb_target}")
    else()
        # Fallback for older STB packages that only provide include dirs
        set(_artifact_stb_include_dir "${Stb_INCLUDE_DIRS}")
        if(NOT _artifact_stb_include_dir)
            set(_artifact_stb_include_dir "${Stb_INCLUDE_DIR}")
        endif()
        if(NOT _artifact_stb_include_dir)
            message(FATAL_ERROR "Stb package found but no include directories provided")
        endif()

        add_library(artifact_stb_manual INTERFACE)
        target_include_directories(artifact_stb_manual INTERFACE ${_artifact_stb_include_dir})
        set(_artifact_stb_target artifact_stb_manual)
        message(STATUS "  ✓ STB:           ${_artifact_stb_include_dir} (header-only)")
    endif()

    # =============================================================================
    # Create unified third-party interface library
    # =============================================================================
    add_library(artifact_third_party INTERFACE)

    target_link_libraries(artifact_third_party INTERFACE
        ${_artifact_glfw_target}
        ${_artifact_glad_target}
        ${_artifact_imgui_target}
        ${_artifact_imguizmo_target}
        ${_artifact_imgui_file_dialog_target}
        ${_artifact_spdlog_target}
        ${_artifact_glm_target}
        ${_artifact_entt_target}
        ${_artifact_efsw_target}
        ${_artifact_tracy_target}
        ${_artifact_box2d_target}
        ${_artifact_jolt_target}
        ${_artifact_assimp_target}
        ${_artifact_stb_target}
    )

    # =============================================================================
    # Platform-specific system libraries
    # =============================================================================
    if(WIN32)
        target_link_libraries(artifact_third_party INTERFACE
            opengl32    # OpenGL
            ws2_32      # Windows Sockets
            winmm       # Windows Multimedia (for timers)
        )
        message(STATUS "  + Windows system libraries linked")

    elseif(APPLE)
        find_library(COCOA_LIBRARY Cocoa REQUIRED)
        find_library(IOKIT_LIBRARY IOKit REQUIRED)
        find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
        find_library(OPENGL_LIBRARY OpenGL REQUIRED)

        target_link_libraries(artifact_third_party INTERFACE
            ${COCOA_LIBRARY}
            ${IOKIT_LIBRARY}
            ${COREVIDEO_LIBRARY}
            ${OPENGL_LIBRARY}
        )
        message(STATUS "  + macOS frameworks linked")

    elseif(UNIX)
        find_package(Threads REQUIRED)
        find_package(X11 REQUIRED)
        find_package(OpenGL REQUIRED)

        target_link_libraries(artifact_third_party INTERFACE
            Threads::Threads
            ${X11_LIBRARIES}
            ${OPENGL_LIBRARIES}
            ${CMAKE_DL_LIBS}
        )
        message(STATUS "  + Linux system libraries linked")
    endif()

    # =============================================================================
    # Tracy profiling support
    # =============================================================================
    if(ARTIFACT_ENABLE_TRACY)
        target_compile_definitions(artifact_third_party INTERFACE TRACY_ENABLE=1)
        message(STATUS "  + Tracy profiling ENABLED")
    else()
        message(STATUS "  + Tracy profiling DISABLED")
    endif()

    message(STATUS "═══════════════════════════════════════════════════════════")
    message(STATUS "  ✓ All dependencies imported successfully")
    message(STATUS "═══════════════════════════════════════════════════════════")
    message(STATUS "")
endfunction()
