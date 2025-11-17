# Continuous Integration

GitHub Actions workflow (`.github/workflows/build.yml`) builds Artifact on Windows, Ubuntu, and macOS. Each job performs:

1. Checkout with submodules (FetchContent dependencies are cloned during configure).
2. Install Ninja + dependencies (Python for GLAD regeneration if needed).
3. Configure using the host-aware preset (`cmake --preset dev-debug`).
4. Build `artifact_editor`, `artifact_runtime`, `artifact_asset_pipeline`, and `artifact_tests`.
5. Run `ctest --preset test-dev`.
6. Upload build artifacts (executables + sample cooked assets) for regression tracking.

## Adding steps
- Extend the matrix in `build.yml` for additional compilers/preset combos.
- Inject environment variables (`ARTIFACT_ENABLE_TRACY=OFF`) if you need stripped builds.
- Use `actions/upload-artifact` to persist `.tracy` captures when working on performance tasks.
