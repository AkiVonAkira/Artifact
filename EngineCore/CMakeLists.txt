set(ENGINECORE_HEADERS
    include/Artifact/Core/Application.h
    include/Artifact/Core/Config.h
    include/Artifact/Core/Instrumentation.h
    include/Artifact/Core/PlayModeController.h
    include/Artifact/Core/Paths.h
    include/Artifact/Assets/AssetWatcher.h
    include/Artifact/Assets/Mesh.h
    include/Artifact/Assets/MeshManager.h
    include/Artifact/Physics/PhysicsSystem.h
    include/Artifact/Rendering/Camera.h
    include/Artifact/Rendering/CameraSystem.h
    include/Artifact/Rendering/Renderer.h
    include/Artifact/Scene/ComponentRegistry.h
    include/Artifact/Scene/Components.h
    include/Artifact/Scene/Prefabs.h
    include/Artifact/Scene/SceneContext.h
    include/Artifact/Scene/SceneService.h
    include/Artifact/Scripting/LuaBindings.h
    include/Artifact/Scripting/LuaContext.h
    include/Artifact/Scripting/LuaRuntimeSystem.h
    include/Artifact/Scripting/ScriptAssetService.h
    include/Artifact/UI/Overlay.h
)

set(ENGINECORE_SOURCES
    Assets/AssetWatcher.cpp
    Assets/MeshManager.cpp
    Core/Application.cpp
    Core/Config.cpp
    Core/PlayModeController.cpp
    Core/Paths.cpp
    Physics/PhysicsSystem.cpp
    Rendering/Camera.cpp
    Rendering/CameraSystem.cpp
    Rendering/Renderer.cpp
    Scene/ComponentRegistry.cpp
    Scene/Prefabs.cpp
    Scene/SceneContext.cpp
    Scene/SceneService.cpp
    Scripting/LuaBindings.cpp
    Scripting/LuaContext.cpp
    Scripting/LuaRuntimeSystem.cpp
    Scripting/ScriptAssetService.cpp
    UI/Overlay.cpp
)

add_library(artifact_engine STATIC ${ENGINECORE_HEADERS} ${ENGINECORE_SOURCES})

add_library(Artifact::EngineCore ALIAS artifact_engine)

target_include_directories(artifact_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_definitions(artifact_engine
    PUBLIC
        ARTIFACT_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(artifact_engine
    PUBLIC
        artifact_third_party
    PRIVATE
        glm::glm
)

artifact_target_enable_warnings(artifact_engine)

if(ARTIFACT_ENABLE_TRACY)
    target_compile_definitions(artifact_engine PUBLIC TRACY_ENABLE=1)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ENGINECORE_HEADERS} ${ENGINECORE_SOURCES})