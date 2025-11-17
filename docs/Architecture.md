# Artifact Engine Architecture

## Vision
Artifact is a cross-platform (Windows, Linux, macOS) OpenGL editor/runtime stack for building moddable games. The initial MVP targets a 3D spline-shooter vertical slice with a Factorio-style Lua modding loop, immediate-mode editor UI, and aggressive hot-reload for scripts, shaders, and assets.

## Technical Pillars
- **Graphics**: OpenGL 3.3 core profile for maximum GPU coverage, with optional 4.5+ feature toggles if a scene requests them. Contexts are bootstrapped through GLFW and GLAD.
- **UI**: Dear ImGui docking branch plus ImGuizmo + ImGuiFileDialog for editor tooling and lightweight in-game overlays.
- **Scripting**: LuaJIT + sol2 bindings with live reload and sandboxing for mods. Scripts can drive ECS systems, input, physics, and rendering parameters.
- **ECS**: EnTT powers scene data, prefab instancing, and serialization.
- **Physics**: Box2D module for 2D gameplay and Jolt Physics for 3D. Both expose a unified component API so gameplay scripts can stay agnostic.
- **Profiling**: Tracy is compiled in by default (editor + runtime). Macros `ZoneScoped` and `FrameMark` are threaded through engine subsystems from day one.
- **File watching**: efsw monitors raw and cooked asset trees via a platform-abstraction layer so every build supports hot reload.

## High-Level Layout
```
Artifact/
├── cmake/                # Custom modules, FetchContent helpers, presets
├── CMakeLists.txt
├── CMakePresets.json
├── docs/
├── EngineCore/
│   ├── Core/             # Application loop, platform layer, job system, config
│   ├── Rendering/        # Renderer, shaders, materials, render graph
│   ├── Scene/            # ECS, prefab loader, serializers
│   ├── Scripting/        # LuaJIT bindings, hot reload manager
│   ├── Physics/          # Box2D + Jolt bridges
│   ├── Assets/           # Import, virtual FS, cook/cache management
│   └── UI/               # Runtime HUD + shared ImGui widgets
├── Editor/
│   ├── Panels/           # Hierarchy, inspector, asset browser, spline tools
│   ├── Viewport/
│   ├── Gizmos/
│   └── PlayMode/         # Play-in-editor bridge
├── Runtime/              # Thin launcher linking EngineCore + selected plugins
├── Plugins/
│   ├── Core/             # Shared plugin helpers
│   ├── SplineShooter/    # MVP gameplay systems (spline editor, AI, scoring)
│   └── Platformer2D/     # Reserved for future work
├── Tools/
│   ├── AssetPipeline/    # Cook on import CLI (textures, models, audio, shaders)
│   ├── Packaging/        # .artifactpkg builder + signing
│   └── Scripts/
├── Games/
│   └── SplineShooterLab/ # Sample project wiring editor ↔ runtime ↔ plugins
├── Assets/
│   ├── Raw/
│   ├── Cooked/
│   └── Shaders/
└── tests/
```

## Editor Experience
- **Layout & Docking**: The editor boots into a Unity-style layout powered by ImGui docking. A main menu and toolbar sit above the dockspace, while Hierarchy, Inspector, Scene, Console, and Assets panels can be rearranged or floated just like Unity’s default workspace.
- **Play Mode Toolbar**: A tri-state toolbar (Play/Pause/Step) toggles between edit and play modes without restarting the application. Play state drives Lua execution while keeping edit-time state in memory, matching Unity’s workflow.
- **Hierarchy & Inspector**: Entities are simple GameObjects with transform data and optional Lua script attachments. The inspector exposes toggles for active state, transform handles, and a Lua component selector with run/reload buttons.
- **Lua-first Dev Loop**: The editor enumerates `Assets/Raw/Scripts`, auto-creates the folder, and ships with `ExampleBehaviour.lua`. Scripts can be assigned to entities, executed on play, and reloaded via a live Lua console input at the bottom of the Console panel.
- **Console & Lua REPL**: Console output shows editor events with filters and auto-scroll, and embeds a REPL-style input that executes arbitrary Lua against the live `LuaContext`.
- **Asset Browser**: A lightweight asset panel lists discovered Lua scripts (future-proofed for other asset types) and supports double-click assignment to the selected entity.

These systems live entirely in the `Editor` target; they register ImGui panels through the shared overlay so the runtime build stays minimal.

## Asset Pipeline Strategy
- **Raw intake**: FBX/GLB/OBJ, PNG/TGA, WAV/FLAC, GLSL, JSON. Stored under `Assets/Raw` and referenced by metadata.
- **Cook on import**: `Tools/AssetPipeline` CLI detects changes (via efsw events or manual command) and:
  - Textures → BC7/ASTC + mip chains using basisu or GPU-compressed exporters.
  - Models → Assimp to engine mesh format (`.artmesh`) with precomputed tangents/LODs.
  - Audio → libogg/libopus, packaged as `.artaudio` with streaming hints.
  - Shaders → GLSLangValidator -> SPIR-V for validation, keep GLSL sources for runtime recompilation. Metadata tracks defines + include graph for hot reload.
- **Cooked output**: Stored under `Assets/Cooked`. Editor/runtime always read cooked data; dev builds can fall back to raw if cook step hasn’t run (with warnings).
- **Packaging**: `artifactpkg` is a manifest-driven zip containing cooked data, Lua modules, and metadata. `cmake --build` can invoke `artifactpkg` generation via custom targets.

## Hot Reload Flow
1. efsw watches both raw assets and Lua/shader directories.
2. On change, AssetPipeline re-cooks if necessary, increments asset version.
3. EngineCore asset manager invalidates GPU resources; renderer rebinds textures/meshes; scripting layer reloads Lua chunk while preserving serialized component state.
4. Tracy markers around reload paths allow profiling of cook + reload latency.

## Packaging Modes
- **Development**: Loose directories, direct file IO, wide-open Lua debugging console, asserts enabled, Tracy live.
- **Shipping**: Assets packed into `.artifactpkg` (zip + manifest + signature). Runtime verifies version, optional encryption/signing hook. Dev toggles disabled, Tracy optional.

## Plugin System
- Each plugin is a shared/static library exposing registration hooks (reflection, ECS systems, Lua APIs).
- `Plugins/SplineShooter` delivers spline editor panels, gameplay components (SplinePath, RailShooterEnemy), AI behaviors, and sample Lua modules.
- Plugin metadata declares dependencies (e.g., SplineShooter depends on EngineCore::Rendering + Physics3D). Games select plugins via manifest; editor auto-loads when opening a project.

## Vertical Slice Roadmap
1. **Scaffold**: Build system, dependencies, Tracy instrumentation, stub renderer/runtime/editor.
2. **Spline Tools**: Implement spline authoring UI, component types, and runtime evaluation.
3. **Gameplay Loop**: AI spawners, scoring, HUD overlay, Lua flow for waves.
4. **Asset Pipeline**: Textures + shader cook, integration with watcher + reload.
5. **Packaging**: Generate `.artifactpkg`, run runtime standalone test.
6. **Secondary Genre**: After shooter MVP stabilizes, add Platformer plugin + sample.

## Testing & CI
- Catch2 tests for math, asset metadata, serialization, plugin registration.
- GPU/integration tests run headless using OSMesa or EGL where available (CI fallback to smoke tests only).
- GitHub Actions matrix (Win/MSVC, Linux/Clang, macOS/Clang) builds editor + runtime + tests, uploads cooked sample artifacts for verification.

## Licensing & Contribution
All rights reserved (custom notice), but contribution guidelines encourage PRs under explicit agreement. Add CLA later if desired.
