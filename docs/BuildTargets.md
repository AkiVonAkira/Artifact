# Build Targets & Options

| Option | Default | Description |
| --- | --- | --- |
| `ARTIFACT_ENABLE_EDITOR` | ON | Build the ImGui editor executable. |
| `ARTIFACT_ENABLE_RUNTIME` | OFF | Build the standalone runtime launcher. |
| `ARTIFACT_ENABLE_TESTS` | ON | Enable Catch2 unit tests (`artifact_tests`). |
| `ARTIFACT_BUILD_PACKAGING` | OFF | Build legacy packaging helpers (enable when you need the CLI). |
| `ARTIFACT_ENABLE_TRACY` | ON | Enable Tracy instrumentation across all targets. |
| `ARTIFACT_ENABLE_TOOLS` | OFF | Build developer CLIs such as the asset pipeline. |

## Targets
- `artifact_engine` – static library with rendering, scripting, physics, hot reload.
- `artifact_editor` – editor UI linking `artifact_engine` + editor panels.
- `artifact_runtime` – thin runtime for packaged games.
- `artifact_plugin_core` / `artifact_plugin_spline` – plugin framework + spline shooter MVP.
- `artifact_asset_pipeline` – cook-on-import CLI.
- `artifact_pkg_builder` – manifest builder for `.artifactpkg` bundles.
- `artifact_tests` – Catch2 suite.

## Presets
| Preset | Generator | Notes |
| --- | --- | --- |
| `editor-debug` | Ninja | Debug build of the editor + tests for the detected host (`build/<Host>/editor-debug`). |
| `editor-release` | Ninja | `RelWithDebInfo` editor build with Tracy disabled for shipping captures. |

Run `cmake --preset editor-debug` to configure, `cmake --build --preset build-editor-debug` to compile, and `ctest --preset test-editor` for tests.

## Dependencies
All third-party packages (GLFW, GLAD, ImGui, LuaJIT, Tracy, etc.) are resolved through the root `vcpkg.json`. Set `VCPKG_ROOT` or place a cloned `vcpkg/` folder next to the repo and the presets will automatically load the vcpkg toolchain, downloading/building the pinned versions into `vcpkg_installed/`. Delete that folder if you need to force a rebuild.
