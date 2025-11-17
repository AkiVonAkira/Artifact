add_executable(artifact_asset_pipeline
    PipelineMain.cpp
)

target_link_libraries(artifact_asset_pipeline PRIVATE
    Artifact::EngineCore
    spdlog
)

artifact_target_enable_warnings(artifact_asset_pipeline)

set_target_properties(artifact_asset_pipeline PROPERTIES FOLDER Tools)
