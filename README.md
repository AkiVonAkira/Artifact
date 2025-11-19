# Artifact Engine

Artifact is a cross-platform OpenGL 3.3+ engine/editor pairing focused on high iteration speed, LuaJIT-driven modding, and a spline-shooter MVP. The project ships with a docking ImGui editor, a slim runtime, a cook-on-import asset pipeline, and a plugin model for gameplay features.

**Version:** 0.1.0 (Active Development)  
**Status:** ⚠️ Undergoing architectural redesign - see [docs/](docs/) for details

## 📚 Documentation

**NEW:** Comprehensive architecture analysis and redesign documentation available!

### Quick Links
- 🎯 **[docs/EXECUTIVE_SUMMARY.md](docs/EXECUTIVE_SUMMARY.md)** - **START HERE** (High-level overview)
- 🚀 **[docs/QUICK_START_CAMERA.md](docs/QUICK_START_CAMERA.md)** - Week 1 implementation guide
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
