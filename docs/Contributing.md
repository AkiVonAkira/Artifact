# Contributing to Artifact

## Branch & PR flow
1. Fork or branch from `main`.
2. Keep feature branches short-lived; rebase on latest `main` before opening a PR.
3. Run `cmake --build --preset build-editor-debug` plus `ctest --preset test-editor` before submitting.
4. Include profiling captures (Tracy `.tracy` file) for performance-sensitive changes.

## Coding guidelines
- C++23, clang-format (Google style, `IndentWidth: 4`, `ColumnLimit: 120`).
- Prefer `std::unique_ptr`/`std::shared_ptr` over raw new/delete.
- Wrap every subsystem entry with `ARTIFACT_ZONE()` and call `ARTIFACT_FRAME_MARK` at the end of loops.
- Use `Artifact::` namespaces by subsystem (`Artifact::Rendering`, `Artifact::Scripting`, etc.).
- Keep headers free of heavy implementation dependencies; forward-declare wherever possible.

## Testing
- Add Catch2 unit tests for new utilities/components in `tests/`.
- For editor-facing additions, add Lua regression scripts under `Games/SplineShooterLab/Scripts/tests` (TODO) and wire them to automation.

## Asset workflow
- Commit only source assets under `Assets/Raw`. Cooked output should be generated during CI/build.
- Lua mods/scripts live under `Games/*/Scripts` and should be small, composable modules.

## Review checklist
- [ ] CMake targets link only what they need.
- [ ] Tracy zones guard hot paths.
- [ ] Asset watcher notifications documented when adding new directory trees.
- [ ] Mods/scripts include inline docs for exposed APIs.
- [ ] CI job passes on Windows/Linux/macOS.
