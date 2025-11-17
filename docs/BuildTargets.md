# Build Targets & Options

| Option | Default | Description |
| --- | --- | --- |
| `ARTIFACT_ENABLE_EDITOR` | ON | Build the ImGui editor executable. |
| `ARTIFACT_ENABLE_RUNTIME` | ON | Build the standalone runtime launcher. |
| `ARTIFACT_ENABLE_TESTS` | ON | Enable Catch2 unit tests (`artifact_tests`). |
| `ARTIFACT_BUILD_PACKAGING` | ON | Build packaging helpers (manifest generator). |
| `ARTIFACT_ENABLE_TRACY` | ON | Enable Tracy instrumentation across all targets. |
| `ARTIFACT_ENABLE_TOOLS` | ON | Build developer CLIs such as the asset pipeline. |

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
| `windows-debug` | Ninja | MSVC v143, Debug. |
| `linux-debug` | Ninja | Clang/GCC, Debug. |
| `macos-debug` | Ninja | AppleClang, Debug. |
| `*-release` | Ninja | `RelWithDebInfo`, unity builds + IPO. |

Run `cmake --preset linux-debug` to configure, `cmake --build --preset build-linux-debug` to compile, and `ctest --preset ctest` for tests.
