# Artifact Engine

Artifact is a cross-platform OpenGL 3.3+ engine/editor pairing focused on high iteration speed, LuaJIT-driven modding, and a spline-shooter MVP. The project ships with a docking ImGui editor, a slim runtime, a cook-on-import asset pipeline, and a plugin model for gameplay features.

**Version:** 0.1.0  
**Status:** School project, no longer in active development. The architectural redesign described in [docs/](docs/) was not finished.

## 🚀 Quick Start

### Building

```bash
# 1. Install vcpkg and set VCPKG_ROOT environment variable
# 2. Configure
cmake --preset debug

# 3. Build
cmake --build --preset debug-build

# 4. Run
./build/debug/bin/<Platform>/Debug/ArtifactEditor
```

**See [docs/BUILD.md](docs/BUILD.md) for detailed build instructions, troubleshooting, and IDE setup.**

## 📚 Documentation

**NEW:** Comprehensive architecture analysis and redesign documentation available!

### Quick Links
- 🏗️ **[docs/BUILD.md](docs/BUILD.md)** - **Building & Dependencies** (vcpkg, CMake, troubleshooting)
- 🎯 **[docs/EXECUTIVE_SUMMARY.md](docs/EXECUTIVE_SUMMARY.md)** - High-level overview
- 🚀 **[docs/QUICK_START_CAMERA.md](docs/QUICK_START_CAMERA.md)** - Week 1 implementation guide
- 🛠️ **[docs/IDE_SETUP.md](docs/IDE_SETUP.md)** - IDE Setup Guide (CLion, VS, VS Code)
- 📖 **[docs/README.md](docs/README.md)** - Full documentation index

### All Documents
- **[docs/ARCHITECTURE_ANALYSIS.md](docs/ARCHITECTURE_ANALYSIS.md)** - Current state assessment
- **[docs/REDESIGN_PROPOSAL.md](docs/REDESIGN_PROPOSAL.md)** - Architectural redesign plan
- **[docs/IMPLEMENTATION_ROADMAP.md](docs/IMPLEMENTATION_ROADMAP.md)** - 6-month implementation timeline
- **[docs/CPP_CAMERA_IMPLEMENTATION.md](docs/CPP_CAMERA_IMPLEMENTATION.md)** - C++ camera system (detailed)
- **[docs/LUA_CPP_BOUNDARIES.md](docs/LUA_CPP_BOUNDARIES.md)** - Lua/C++ design philosophy
- **[docs/PACKAGE_MANAGER_COMPARISON.md](docs/PACKAGE_MANAGER_COMPARISON.md)** - vcpkg alternatives

## Highlights
- **Platforms**: Windows, Linux, macOS.
- **Graphics**: OpenGL 3.3 core for compatibility, optional 4.5 extensions, GLAD + GLFW bootstrap, Tracy instrumentation wired from frame one.
- **Editor**: ImGui docking UI, gizmo and spline panels, console, Catch2 Test runner, profiler overlay, Play-In-Editor loop.
- **Runtime**: Minimal launcher that loads loose assets in dev or `.artifactpkg` bundles for shipping.
- **Scripting**: LuaJIT + sol2 with hot reload, Factorio-style mod folders, command console.
- **Asset pipeline**: efsw-backed hot reload, cook-on-import CLI (BC7/ASTC compression, Assimp import, GLSL validation).
- **Physics**: Box2D (2D) + Jolt (3D) behind a unified component API.
- **ECS**: EnTT for data-driven architecture, serialization, and eventing.
- **UI**: ImGui with custom theming, docking, and editor widgets.
- **Testing**: Catch2 unit tests, GitHub Actions matrix for Win/Linux/mac.

## License

Copyright (C) 2025-2026 Ilyas Kaya. Licensed under the GNU Affero General Public License v3.0 only, see [LICENSE](LICENSE). For a commercial license without the AGPL terms, contact me@kayzel.com.
