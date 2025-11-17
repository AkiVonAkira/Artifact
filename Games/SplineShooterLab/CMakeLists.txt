set(SPLINE_GAME_SOURCES
    GameModule.cpp
)

add_library(artifact_game_spline STATIC ${SPLINE_GAME_SOURCES})
add_library(Artifact::GameSplineShooter ALIAS artifact_game_spline)

target_link_libraries(artifact_game_spline
    PUBLIC
        Artifact::EngineCore
        Artifact::PluginSplineShooter
)

artifact_target_enable_warnings(artifact_game_spline)

set_target_properties(artifact_game_spline PROPERTIES FOLDER Games)
