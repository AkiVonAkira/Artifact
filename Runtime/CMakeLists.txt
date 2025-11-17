add_executable(artifact_runtime main.cpp)

target_link_libraries(artifact_runtime PRIVATE Artifact::EngineCore)

target_compile_definitions(artifact_runtime PRIVATE ARTIFACT_IS_RUNTIME=1)

artifact_target_enable_warnings(artifact_runtime)

set_target_properties(artifact_runtime PROPERTIES FOLDER Runtime)
