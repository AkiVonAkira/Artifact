find_package(Catch2 CONFIG REQUIRED)

set(EDITOR_HEADERS
    EditorLayer.h
    EditorState.h
    Panels/Panel.h
    Panels/PanelFactories.h
    Panels/PanelInstances.h
    Panels/PanelRegistry.h
    Viewport/EditorCameraController.h
)

set(EDITOR_SOURCES
    main.cpp
    EditorLayer.cpp
    Panels/AssetsPanel.cpp
    Panels/ConsolePanel.cpp
    Panels/HierarchyPanel.cpp
    Panels/InspectorPanel.cpp
    Panels/MainMenuPanel.cpp
    Panels/ScenePanel.cpp
    Panels/TestRunnerPanel.cpp
    Panels/PanelInstances.cpp
    Panels/PanelRegistry.cpp
    Viewport/EditorCameraController.cpp
)

add_executable(artifact_editor ${EDITOR_HEADERS} ${EDITOR_SOURCES})

target_include_directories(artifact_editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Panels
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewport
)

target_link_libraries(artifact_editor PRIVATE
    Artifact::EngineCore
    Catch2::Catch2
)

target_compile_definitions(artifact_editor PRIVATE ARTIFACT_IS_EDITOR=1)

artifact_target_enable_warnings(artifact_editor)

set_target_properties(artifact_editor PROPERTIES FOLDER Editor)

add_custom_command(TARGET artifact_editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:artifact_editor>/Assets)
