set(EDITOR_SOURCES
    main.cpp
    EditorLayer.cpp
)

add_executable(artifact_editor ${EDITOR_SOURCES})

target_link_libraries(artifact_editor PRIVATE Artifact::EngineCore)

target_compile_definitions(artifact_editor PRIVATE ARTIFACT_IS_EDITOR=1)

artifact_target_enable_warnings(artifact_editor)

set_target_properties(artifact_editor PROPERTIES FOLDER Editor)
