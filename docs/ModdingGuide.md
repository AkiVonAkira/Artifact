# Modding Guide

Artifact mods follow a Factorio-style structure: a folder with metadata, Lua scripts, assets, and optional binaries packaged into `.artifactpkg` or referenced loosely during development.

## Folder layout
```
Games/
  SplineShooterLab/
    Scripts/
      main.lua
      mods/
        my_mod/
          modinfo.lua
          control.lua
          data.lua
    Assets/
      Raw/
      Cooked/
```

`modinfo.lua` example:
```lua
return {
  name = "my_mod",
  version = "0.0.1",
  description = "Experimental railgun variant",
  dependencies = { "base >= 0.1.0", "spline_core" }
}
```

## Hot reload loop
1. Editor launches Lua watcher (efsw) over `Games/*/Scripts`.
2. On file change the watcher queues the module name.
3. `LuaContext` invalidates `package.loaded[module]` and re-requires it.
4. Registered callbacks (editor panels, runtime systems) patch live component state.

## APIs exposed to Lua
- `artifact.scene.spawn(prefab_name, transform)`
- `artifact.physics.apply_impulse(entity, vec3)`
- `artifact.spline.query(pathId, t)`
- `artifact.ui.toast(text, duration)`

(See `LuaContext::registerBindings` TODO for detailed signatures.)

## Packaging mods
1. Run `artifact_asset_pipeline --raw Games/SplineShooterLab/Assets --cooked Assets/Cooked --shaders=on` to cook resources referenced by the mod.
2. Create manifest stub:
   ```pwsh
   artifact_pkg_builder --name my_mod --version 0.0.1 --cooked Assets/Cooked --out dist
   ```
3. Zip cooked payload + manifest to `my_mod.artifactpkg` (CI automation pending).
4. Ship `.artifactpkg` alongside runtime. The runtime reads manifest, validates signatures (TODO), and mounts the package.

## Scripting best practices
- Keep gameplay logic pure; call into C++ for expensive operations only.
- Use `artifact.events.on("enemy_killed", function(args) ... end)` style to decouple systems.
- Prefer data-driven definitions (JSON/TOML) for weapon stats; use Lua for orchestration.
- Tag modules with `return function(api) ... end` to support dependency injection/mocking.
