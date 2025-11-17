# Game Templates

## SplineShooterLab (MVP)
- Located at `Games/SplineShooterLab`.
- Depends on `Artifact::PluginSplineShooter`.
- Assets live alongside scripts for fast iteration.
- Provides spline editor panel + runtime behaviors.

### Creating a new game from the template
1. Copy `Games/SplineShooterLab` to `Games/<YourGame>`.
2. Update `Games/CMakeLists.txt` to add the new folder.
3. Rename the module/library in the local `CMakeLists.txt`.
4. Update Lua bootstrap script under `Scripts/`.
5. Register new plugins or dependencies via manifest (TODO: `artifact_project.yml`).

## Platformer (future)
- Lives under `Plugins/Platformer2D` (stub for now).
- Shares asset pipeline + watchers; physics defaults to Box2D.
- Add plugin to `Games/<Platformer>` when ready.

## Recommendations
- Keep gameplay-specific assets and scripts scoped to the game folder, but reuse shared shaders/materials under `Assets/`.
- Place per-game configuration in `game.toml` (TODO) and load via `Artifact::Core::LoadRuntimeConfig`.
- Use plugins for reusable features; games should mainly contain glue/config + Lua modules.
