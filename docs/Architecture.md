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
│   ├── Tilemap2D/        # 2D tilemap editor + runtime components
│   ├── Camera3D/         # 3D camera controller + editor gizmos
│   ├── Physics3D/        # Jolt Physics plugin exposing 3D physics components
│   ├── SplineShooter/    # Spline shooter gameplay, AI, HUD, sample scripts
│   ├── BehaviorTree/     # Behavior tree editor + runtime evaluation
│   └── Spline/           # Spline editor, components, runtime evaluation
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

## Lua Behaviour Lifecycle
- **Awake**: Called the first frame an entity’s script is instantiated or reloaded. `LuaContext` allocates the behaviour, injects component handles, and runs `Behaviour:Awake()`. Use this for cache-friendly lookups, registering Tracy zones, and binding to engine singletons. The editor invokes Awake immediately when you assign a script so inspectors can reflect freshly created fields.
- **Start**: Fired on the first Play-mode tick after Awake completes. Use `Start()` for gameplay wiring that should only run once the world is live (spawning projectiles, scheduling timers, sending analytics events). When hot-reloading, the engine re-runs Awake and queues Start only if the behaviour did not already begin play, mirroring Unity’s semantics.
- **Update(delta_time)**: Runs every frame while the entity is active and Play mode is engaged. Update receives the timestep from `EngineCore::Core::Application` and can call into ECS, physics, or rendering subsystems through the sol2 bindings. If Pause is pressed, Update stops; Step executes a single Update.
- **Hot reload expectations**: `Assets/Raw/Scripts/ExampleBehaviour.lua` shows the preferred Awake/Start/Update hooks so mods can opt in gradually. When a file changes, LuaContext tears down the old chunk, rebinds serialized state, calls Awake, conditionally Start, and continues Update without requiring a restart. Future lifecycle hooks (e.g., `OnDestroy`, `OnEnable/OnDisable`) will follow the same pattern.

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

## Engine Foundations Status
- **Rendering** (`EngineCore/Rendering`, `Assets/Shaders`): Core OpenGL scaffolding, ImGui viewport, and shader hot-reload entry points exist, but render graph abstraction and material system authoring still need validation against heavier content.
- **Scene & ECS** (`EngineCore/Scene` + EnTT usage): Entity lifecycle is wired to the editor, yet prefab serialization, undo/redo, and scene versioning remain work-in-progress; scripting relies on consistent component schemas that are only partially enforced.
- **Scripting** (`EngineCore/Scripting/LuaContext.cpp`, `Assets/Raw/Scripts`): LuaJIT + sol2 integration plus lifecycle hooks are in place, though API surface documentation, safety sandboxes, and coverage of engine subsystems (physics, UI) must expand before shipping mods.
- **Physics** (`EngineCore/Physics`, `Plugins/Physics3D`): Bridges for Box2D and Jolt compile, but there is no unified authoring workflow in the editor yet (gizmos, debug draw, component inspectors).
- **Asset pipeline & tooling** (`Tools/AssetPipeline`, `Assets/Cooked`): CLI stubs exist with watcher integration, yet automated cook graphs, dependency hashing, and error surfacing inside the editor UI remain gaps.
- **Testing & CI** (`tests/`, `docs/CI.md`): Catch2 harness is bootstrapped, but there are few assertions around scripting, serialization, or renderer state; CI plans exist on paper but not all presets are proven.

## Next Development Steps
1. Modularize editor UX: split `EditorLayer` responsibilities into dedicated panels, add dockspace layout presets, and surface contextual shortcuts (duplicate, focus, search) to keep iteration fluid.
2. Build an interactive scene viewport: render the runtime framebuffer inside the Scene panel using `EngineCore::Rendering`, add gizmo scaffolding, and support viewport resizing + aspect locking.
3. Synchronize Play-in-Editor with runtime systems: drive Lua lifecycle, physics, and renderer ticks from editor play/pause/step controls so gameplay seen in the viewport matches standalone builds.
4. Harden the scripting surface: document all sol2 bindings, add regression tests for lifecycle reloads, and expose safe wrappers for physics/rendering calls so Lua mods cannot crash the engine.
5. Formalize scene data: implement prefab serialization + GUID stable IDs in `EngineCore/Scene`, wire undo/redo in the editor, and ensure Lua behaviours survive scene reloads without orphaned references.
6. Expand asset pipeline automation: hook `Tools/AssetPipeline` into the editor with progress/error surfacing, add dependency hashing for shaders/models, and produce deterministic cook outputs for CI.
7. Build physics authoring UX: create Box2D/Jolt inspectors, gizmos, and debug overlays under `Editor/` so designers can validate collision setups without custom tooling.
8. Render architecture validation: prototype the forward+ or clustered renderer path with representative content, bake material/shader libraries, and lock down the render graph API so plugins can register passes predictably.
9. Align editor/runtime parity: ensure Play-in-Editor matches standalone `Runtime/` by sharing launch arguments, input stacks, and save data paths; add automated smoke tests that boot both binaries with the same sample project to catch drift.
10. Strengthen CI + perf budgets: extend `docs/CI.md` with preset coverage expectations, add GPU mock tests for shader compilation, and gate merges on Tracy capture budgets to keep frame time headroom visible.

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

## Implementation Notes & Open Questions
- **Lifecycle guarantees**: Awake is called immediately when a script is assigned in the editor, and Start is queued for the first Play tick. This allows inspectors to reflect newly created fields right away.
- **Lua sandboxing**: Initial exposure is via high-level engine wrappers only. Direct OpenGL or low-level access may be considered later if there is a strong use case.
- **Prefab GUID scheme**: UUIDv4 is chosen for simplicity and uniqueness. While less user-friendly in diffs, it avoids collision risks in collaborative environments.
- **Prefab serialization format**: Flat JSON is selected for its ease of iteration and diffability during development. Performance optimizations can be revisited later if load times become a bottleneck.
- **Physics debug draw**: ImGui draw lists are used for initial implementation due to faster shipping. Performance optimizations can be explored later if necessary.
- **Asset cook orchestration**: `cmake --build` custom targets are preferred for their integration with the existing build system. This approach will be documented in `Tools/AssetPipeline/README`.
- **CI infrastructure**: Self-hosted runners handle GPU validation as needed. The matrix will scale based on resource availability and coverage requirements.
- **Rendering backend roadmap**: OpenGL remains the active backend. Vulkan should be pursued only once the editor UX solidifies, ideally as a drop-in option surfaced through an engine setting so projects can toggle without code changes.
