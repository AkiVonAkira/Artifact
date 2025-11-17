# Continuous Integration

GitHub Actions workflow (`.github/workflows/build.yml`) builds Artifact on Windows, Ubuntu, and macOS. Each job performs:

1. Checkout the repo (the vcpkg manifest resolves dependencies during configure).
2. Install Ninja + dependencies (Python for GLAD regeneration if needed).
3. Configure using the editor preset (`cmake --preset editor-debug`).
4. Build `artifact_editor` + `artifact_tests` (runtime/tools stay disabled unless explicitly re-enabled).
5. Run `ctest --preset test-editor`.
6. Upload the editor/test binaries for regression tracking.

## Adding steps
- Extend the matrix in `build.yml` for additional compilers/preset combos.
- Inject environment variables (`ARTIFACT_ENABLE_TRACY=OFF`) if you need stripped builds.
- Use `actions/upload-artifact` to persist `.tracy` captures when working on performance tasks.
