find_package(Catch2 CONFIG REQUIRED)

add_executable(artifact_tests
    test_main.cpp
    test_config.cpp
    test_scene.cpp
    test_lua_bindings.cpp
)

target_link_libraries(artifact_tests PRIVATE
    Artifact::EngineCore
    Catch2::Catch2
)

artifact_target_enable_warnings(artifact_tests)

add_test(NAME artifact_tests COMMAND artifact_tests)
