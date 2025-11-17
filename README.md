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
   - Python 3 (only needed if you plan to regenerate GLAD bindings)
   - A C++23-compatible compiler (MSVC v143, Clang 16+, or GCC 12+)
2. **Configure**
   ```pwsh
   cmake --preset windows-debug
   ```
3. **Build**
   ```pwsh
   cmake --build --preset build-windows-debug
   ```
4. **Run**
   ```pwsh
   .\build\windows-debug\artifact_editor.exe
   ```

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
- **Dev**: Use loose directories, live reload, asserts on.
- **Ship**: Run `artifact_asset_pipeline` then `artifact_pkg_builder --name SplineShooter --version 0.1.0` to emit `.artifactpkg.manifest`. Future work will zip/sign assets automatically.

## CI
GitHub Actions workflow (`.github/workflows/build.yml`) configures/pipelines builds for Windows/MSVC, Ubuntu/Clang, and macOS/Clang. Each job configures via presets, builds editor/runtime/tests, and executes the Catch2 suite.

## License
All rights reserved. See `LICENSE`. Contributions are welcomed via pull requests; by submitting a contribution you agree to transfer necessary rights to the project owner.
