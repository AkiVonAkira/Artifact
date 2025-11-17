add_library(artifact_plugin_core INTERFACE)
add_library(Artifact::PluginCore ALIAS artifact_plugin_core)

target_include_directories(artifact_plugin_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
