# Artifact Engine

Artifact is a cross-platform OpenGL 3.3+ engine/editor pairing focused on high iteration speed, LuaJIT-driven modding, and a spline-shooter MVP. The project ships with a docking ImGui editor, a slim runtime, a cook-on-import asset pipeline, and a plugin model for gameplay features.

## Highlights
- **Platforms**: Windows, Linux, macOS.
- **Graphics**: OpenGL 3.3 core for compatibility, optional 4.5 extensions, GLAD + GLFW bootstrap, Tracy instrumentation wired from frame one.
- **Editor**: ImGui docking UI, gizmo and spline panels, Lua console, profiler overlay, Play-In-Editor loop.
- **Runtime**: Minimal launcher that loads loose assets in dev or `.artifactpkg` bundles for shipping.
- **Scripting**: LuaJIT + sol2 with hot reload, Factorio-style mod folders, command console.
- **Asset pipeline**: efsw-backed hot reload, cook-on-import CLI, BC7/ASTC texture compression, Assimp model import, GLSL → SPIR-V validation.
- **Physics**: Box2D (2D) + Jolt (3D) behind a unified component API.
- **Testing**: Catch2 unit tests, GitHub Actions matrix for Win/Linux/mac.

## Getting started
1. **Install prerequisites**
   - CMake 3.24+
   - Ninja (recommended)
   - [vcpkg](https://github.com/microsoft/vcpkg) (set `VCPKG_ROOT` or place a cloned `vcpkg/` folder next to the repo; the presets pick it up automatically)
   - Python 3 (only needed if you plan to regenerate GLAD bindings)
   - A C++23-compatible compiler (MSVC v143, Clang 16+, or GCC 12+)
2. **Configure** (auto-detects your host platform)
   ```pwsh
   cmake --preset editor-debug
   ```
3. **Build**
   ```pwsh
   cmake --build --preset build-editor-debug
   ```
4. **Run** – binaries land under `build/<HostSystemName>/editor-debug`. On Windows that resolves to `build/Windows/editor-debug`:
   ```pwsh
   .\build\Windows\editor-debug\artifact_editor.exe
   ```
   On Linux/macOS replace `Windows` with `Linux` or `Darwin` respectively when launching from the terminal.

## IDE shortcuts
- **Visual Studio Code** – the bundled settings default to the `editor-debug` preset and automatically invoke the `build-editor-debug` task before launching the editor target.
- **JetBrains Rider / CLion** – select the `editor-debug` CMake profile for day-to-day work. Release profiling builds are available via `editor-release` if you need RelWithDebInfo binaries without Tracy.

## Dependency management
Artifact now relies on the `vcpkg` manifest (`vcpkg.json`) for GLFW, GLAD, ImGui, LuaJIT, Tracy, and the rest of the third-party stack. Set `VCPKG_ROOT` (or drop a cloned `vcpkg/` folder next to the repo) and the bundled presets/toolchain will automatically locate `scripts/buildsystems/vcpkg.cmake` and install the pinned dependencies into `vcpkg_installed/`.

## Repo layout
- `EngineCore/` – rendering, ECS, Lua, physics, asset IO, UI wrappers.
- `Editor/` – GLFW/ImGui shell, panels, gizmos, play mode bridge.
- `Runtime/` – standalone executable for packaged games.
- `Plugins/` – optional feature packs (SplineShooter MVP included).
- `Games/` – sample game modules built on top of plugins.
- `Tools/AssetPipeline` – cook-on-import CLI.
- `Tools/Packaging` – manifest/packaging helper.
- `Assets/Raw|Cooked|Shaders` – default asset folders (watcher-aware).
- `docs/` – architecture, contributing, modding & build guides.

## Hot reload workflow
1. Start the editor (`artifact_editor`).
2. Asset watcher (efsw) tracks `Assets/Raw` and `Games/*/Scripts`.
3. Modify a Lua script or shader – the watcher queues a reload.
4. Lua context invalidates the module and re-executes while preserving serialized state.
5. Shaders trigger GLSL compile + SPIR-V validation before re-binding to the renderer.

## Packaging
- **Dev**: Stay inside the editor—use loose directories, live reload, asserts on.
- **Ship**: Use the in-editor packaging panel to cook content and generate `.artifactpkg` bundles directly from the running editor session. The legacy CLI helpers (`artifact_asset_pipeline`, `artifact_pkg_builder`) remain available if you re-enable `ARTIFACT_ENABLE_TOOLS` / `ARTIFACT_BUILD_PACKAGING`, but they’re no longer part of the default build.

## CI
GitHub Actions workflow (`.github/workflows/build.yml`) configures/pipelines builds for Windows/MSVC, Ubuntu/Clang, and macOS/Clang. Each job uses the `editor-debug` preset, builds the editor + tests, and runs the Catch2 suite. Re-enable optional components (runtime/tools/packaging) via cache variables if your fork needs them.

## License
All rights reserved. See `LICENSE`. Contributions are welcomed via pull requests; by submitting a contribution you agree to transfer necessary rights to the project owner.
