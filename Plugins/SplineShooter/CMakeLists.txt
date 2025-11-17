set(SPLINE_PLUGIN_SOURCES
    src/SplineShooterPlugin.cpp
    include/Artifact/Plugins/SplineShooterPlugin.h
)

add_library(artifact_plugin_spline STATIC ${SPLINE_PLUGIN_SOURCES})
add_library(Artifact::PluginSplineShooter ALIAS artifact_plugin_spline)

target_include_directories(artifact_plugin_spline PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(artifact_plugin_spline
    PUBLIC
        Artifact::EngineCore
        Artifact::PluginCore
)

artifact_target_enable_warnings(artifact_plugin_spline)

set_target_properties(artifact_plugin_spline PROPERTIES FOLDER Plugins)
