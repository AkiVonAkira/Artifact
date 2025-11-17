include(FetchContent)

function(artifact_fetch_third_party)

    set(FETCHCONTENT_QUIET OFF)
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

    # GLFW
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )

    # GLAD (pre-generated core profile loader)
    FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG 73db193
        SOURCE_SUBDIR cmake
    )

    # ImGui docking branch
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
    )

    # ImGuizmo
    FetchContent_Declare(
        imguizmo
        GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
        GIT_TAG 664cf2d
    )

    # ImGuiFileDialog (header only)
    FetchContent_Declare(
        imgui_file_dialog
        GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
        GIT_TAG v0.6.6
    )

    # spdlog
    set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG 486b555
    )

    # glm
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG a532f5b
    )

    # EnTT
    FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG d4014c7
    )

    # sol2
    FetchContent_Declare(
        sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG eba8662
    )

    # LuaJIT source (used by the CMake wrapper)
    FetchContent_Declare(
        luajit_source
        GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
        GIT_TAG 2090842
    )

    # LuaJIT (CMake-friendly wrapper consuming LUAJIT_DIR)
    set(LUAJIT_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        luajit_cmake
        GIT_REPOSITORY https://github.com/zhaozg/luajit-cmake.git
    )

    # efsw
    FetchContent_Declare(
        efsw
        GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
        GIT_TAG 1.3.1
    )

    # Tracy
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.11.1
    )

    # Box2D
    set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        box2d
        GIT_REPOSITORY https://github.com/erincatto/box2d.git
        GIT_TAG v3.0.0
    )

    # Jolt Physics
    set(JPH_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(JPH_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        jolt
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.0.0
    )

    # Assimp
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.3.1
    )

    # Catch2
    FetchContent_Declare(
        catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG b3fb4b9
    )

    # stb image (header only)
    FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
    )

    FetchContent_GetProperties(luajit_source)
    if(NOT luajit_source_POPULATED)
        if(POLICY CMP0169)
            cmake_policy(PUSH)
            cmake_policy(SET CMP0169 OLD)
        endif()
        FetchContent_Populate(luajit_source
            SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/luajit_source-src
            BINARY_DIR ${FETCHCONTENT_BASE_DIR}/luajit_source-build
            GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
            GIT_TAG 2090842
        )
        if(POLICY CMP0169)
            cmake_policy(POP)
        endif()
    endif()
    set(LUAJIT_DIR ${luajit_source_SOURCE_DIR} CACHE PATH "Path to upstream LuaJIT sources used by luajit-cmake" FORCE)

    FetchContent_MakeAvailable(glfw glad glm spdlog entt luajit_cmake efsw box2d jolt assimp catch2 tracy)

    set(_ARTIFACT_PYTHON_PATH "${Python_EXECUTABLE}")
    if(NOT _ARTIFACT_PYTHON_PATH OR _ARTIFACT_PYTHON_PATH STREQUAL "Python_EXECUTABLE-NOTFOUND")
        if(NOT WIN32)
            find_program(_ARTIFACT_PYTHON_PATH NAMES python3 python)
        endif()
    endif()

    if((NOT _ARTIFACT_PYTHON_PATH OR _ARTIFACT_PYTHON_PATH STREQUAL "Python_EXECUTABLE-NOTFOUND") AND WIN32)
        set(ARTIFACT_EMBED_PYTHON_VERSION 3.12.1)
        set(_artifact_python_embed_root ${CMAKE_BINARY_DIR}/artifact-python-${ARTIFACT_EMBED_PYTHON_VERSION})
        set(_artifact_python_embed_zip ${_artifact_python_embed_root}.zip)
        if(NOT EXISTS ${_artifact_python_embed_root}/python.exe)
            file(MAKE_DIRECTORY ${_artifact_python_embed_root})
            file(DOWNLOAD
                https://www.python.org/ftp/python/${ARTIFACT_EMBED_PYTHON_VERSION}/python-${ARTIFACT_EMBED_PYTHON_VERSION}-embed-amd64.zip
                ${_artifact_python_embed_zip}
                SHOW_PROGRESS
            )
            file(ARCHIVE_EXTRACT INPUT ${_artifact_python_embed_zip} DESTINATION ${_artifact_python_embed_root})
        endif()
        set(_ARTIFACT_PYTHON_PATH ${_artifact_python_embed_root}/python.exe)
    endif()

    if(_ARTIFACT_PYTHON_PATH AND NOT _ARTIFACT_PYTHON_PATH STREQUAL "Python_EXECUTABLE-NOTFOUND")
        set(Python_EXECUTABLE ${_ARTIFACT_PYTHON_PATH} CACHE FILEPATH "Python interpreter used for GLAD code generation" FORCE)
        set(Python3_EXECUTABLE ${_ARTIFACT_PYTHON_PATH} CACHE FILEPATH "" FORCE)
    elseif(NOT _ARTIFACT_PYTHON_PATH)
        message(FATAL_ERROR "Python interpreter not found. Install Python 3 or set Python_EXECUTABLE before configuring.")
    endif()

    if(COMMAND glad_add_library)
        if(NOT TARGET glad)
            set(ARTIFACT_GLAD_API "gl:core=4.6" CACHE STRING "GLAD API profile")
            glad_add_library(glad STATIC REPRODUCIBLE LOADER API ${ARTIFACT_GLAD_API})
        endif()
    else()
        message(FATAL_ERROR "glad_add_library function not available; ensure GLAD FetchContent succeeded.")
    endif()

    if(TARGET glm AND NOT TARGET glm::glm)
        add_library(glm::glm ALIAS glm)
    endif()

    if(TARGET spdlog::spdlog_header_only)
        set(ARTIFACT_SPDLOG_TARGET spdlog::spdlog_header_only)
    elseif(TARGET spdlog::spdlog)
        set(ARTIFACT_SPDLOG_TARGET spdlog::spdlog)
    else()
        message(FATAL_ERROR "spdlog target was not generated. Check FetchContent declaration.")
    endif()

    if(TARGET libluajit AND NOT TARGET LuaJIT::LuaJIT)
        add_library(LuaJIT::LuaJIT ALIAS libluajit)
    endif()

    if(TARGET efsw AND NOT TARGET efsw::efsw)
        add_library(efsw::efsw ALIAS efsw)
    elseif(TARGET efsw-static AND NOT TARGET efsw::efsw)
        add_library(efsw::efsw ALIAS efsw-static)
    endif()



    if(TARGET box2d AND NOT TARGET Box2D::Box2D)
        add_library(Box2D::Box2D ALIAS box2d)
    endif()

    if(TARGET assimp AND NOT TARGET assimp::assimp)
        add_library(assimp::assimp ALIAS assimp)
    endif()

    if(NOT TARGET Catch2::Catch2WithMain AND TARGET Catch2WithMain)
        add_library(Catch2::Catch2WithMain ALIAS Catch2WithMain)
    endif()

    if(NOT TARGET Catch2::Catch2 AND TARGET Catch2)
        add_library(Catch2::Catch2 ALIAS Catch2)
    endif()

    # sol2 header-only target
    FetchContent_GetProperties(sol2)
    # Only populate sol2 if it wasn't populated AND there isn't already a target named "sol2"
    if(NOT sol2_POPULATED AND NOT TARGET sol2)
        FetchContent_MakeAvailable(sol2)
    endif()
    if(TARGET sol2 AND NOT TARGET artifact_sol2)
        add_library(artifact_sol2 INTERFACE)
        target_link_libraries(artifact_sol2 INTERFACE sol2)
        target_include_directories(artifact_sol2 INTERFACE ${sol2_SOURCE_DIR}/single/include ${sol2_SOURCE_DIR}/include)
    elseif(NOT TARGET artifact_sol2)
        # If the sol2 project doesn't create a target, create a local interface target that supplies includes
        add_library(artifact_sol2 INTERFACE)
        target_include_directories(artifact_sol2 INTERFACE ${sol2_SOURCE_DIR}/single/include ${sol2_SOURCE_DIR}/include)
    endif()
    
    # ImGui setup
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_MakeAvailable(imgui)
        set(IMGUI_SOURCES
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )
        add_library(imgui STATIC ${IMGUI_SOURCES})
        target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC glfw glad)
        target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
    endif()

    FetchContent_GetProperties(imguizmo)
    if(NOT imguizmo_POPULATED)
        FetchContent_MakeAvailable(imguizmo)
        add_library(ImGuizmo STATIC ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp)
        target_include_directories(ImGuizmo PUBLIC ${imguizmo_SOURCE_DIR})
        target_link_libraries(ImGuizmo PUBLIC imgui)
    endif()

    FetchContent_GetProperties(imgui_file_dialog)
    if(NOT imgui_file_dialog_POPULATED)
        FetchContent_MakeAvailable(imgui_file_dialog)
    endif()
    if(TARGET ImGuiFileDialog)
        target_include_directories(ImGuiFileDialog PUBLIC ${imgui_file_dialog_SOURCE_DIR})
    endif()

    # stb header-only target
    FetchContent_GetProperties(stb)
    if(NOT stb_POPULATED)
        FetchContent_MakeAvailable(stb)
        add_library(stb INTERFACE)
        target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
    endif()

    add_library(artifact_third_party INTERFACE)
    target_link_libraries(artifact_third_party INTERFACE
        glfw
        glad
        imgui
        ImGuizmo
        ImGuiFileDialog
        ${ARTIFACT_SPDLOG_TARGET}
        glm::glm
        EnTT::EnTT
        artifact_sol2
        LuaJIT::LuaJIT
        efsw::efsw
        TracyClient
        Box2D::Box2D
        Jolt
        assimp::assimp
        stb
    )

    if(ARTIFACT_ENABLE_TRACY)
        target_compile_definitions(artifact_third_party INTERFACE TRACY_ENABLE=1)
    endif()

endfunction()
