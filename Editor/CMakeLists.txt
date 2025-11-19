set(EDITOR_SOURCES
        main.cpp
        EditorLayer.cpp
        EditorLayer.h
        EditorState.h
        Panels/Panel.h
        Panels/PanelFactories.h
        Panels/AssetsPanel.cpp
        Panels/ConsolePanel.cpp
        Panels/HierarchyPanel.cpp
        Panels/InspectorPanel.cpp
        Panels/MainMenuPanel.cpp
        Panels/ScenePanel.cpp
        Viewport/EditorCameraController.cpp
        Viewport/EditorCameraController.h
)

add_executable(artifact_editor ${EDITOR_SOURCES})

target_include_directories(artifact_editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Panels
)

target_link_libraries(artifact_editor PRIVATE Artifact::EngineCore)

target_compile_definitions(artifact_editor PRIVATE ARTIFACT_IS_EDITOR=1)

artifact_target_enable_warnings(artifact_editor)

set_target_properties(artifact_editor PROPERTIES FOLDER Editor)
