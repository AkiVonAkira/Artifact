# Artifact Engine - Architecture Analysis
**Date:** November 19, 2025  
**Version:** 0.1.0
## Executive Summary
Artifact is a cross-platform OpenGL 3.3+ game engine with an integrated editor, featuring LuaJIT scripting, ECS architecture (EnTT), and a plugin system. The engine is designed for high iteration speed with hot-reloading, docking ImGui editor, and a modular plugin architecture suitable for 2D/3D games.
---
## Current Architecture Overview
### High-Level Structure
```
┌─────────────────────────────────────────────────────────────┐
│                    Artifact Engine                          │
├─────────────────────────────────────────────────────────────┤
│  ┌────────────┐  ┌────────────┐  ┌────────────┐           │
│  │   Editor   │  │  Runtime   │  │   Tools    │           │
│  │  (ImGui)   │  │ (Launcher) │  │ (Pipeline) │           │
│  └─────┬──────┘  └─────┬──────┘  └─────┬──────┘           │
│        │               │               │                    │
│  ──────┴───────────────┴───────────────┴──────────────     │
│        │         EngineCore (Static Lib)                    │
│  ──────┴────────────────────────────────────────────────   │
│                                                              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐     │
│  │  Scene   │ │ Scripting│ │Rendering │ │ Physics  │     │
│  │ (EnTT)   │ │ (LuaJIT) │ │ (OpenGL) │ │Box2D/Jolt│     │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘     │
│                                                              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐                   │
│  │  Assets  │ │    UI    │ │ Plugins  │                   │
│  │  (efsw)  │ │ (ImGui)  │ │ (Dynamic)│                   │
│  └──────────┘ └──────────┘ └──────────┘                   │
└─────────────────────────────────────────────────────────────┘
```
---
## Module Breakdown
### 1. **EngineCore** (Foundation Layer)
**Location:** `EngineCore/`
#### Sub-Systems:
##### Core
- **Application.cpp/h**: Main application lifecycle, window management (GLFW), ImGui initialization
- **Config.cpp/h**: Runtime configuration (paths, settings)
- **PlayModeController.h**: Play/Edit mode state management
- **Instrumentation.h**: Tracy profiling integration
##### Rendering
- **Renderer.cpp/h**: OpenGL rendering abstraction
  - Scene framebuffer management
  - Skybox rendering (procedural gradient)
  - Basic OpenGL state management
  - **Limitation**: No camera system implemented in C++
##### Scene
- **SceneService.cpp/h**: High-level scene management API
- **SceneContext.cpp/h**: EnTT registry wrapper
- **Components.h**: ECS component definitions
  - Transform, Name, ID, Active
  - **LuaScriptComponent**: Links entities to Lua scripts
- **Prefabs.cpp/h**: Entity template system
##### Scripting
- **LuaContext.cpp/h**: Sol2/LuaJIT wrapper, hot-reload queue
- **LuaBindings.cpp/h**: C++ ↔ Lua API bindings
  - Input (keyboard queries)
  - Entity transforms (translate, rotate, position)
  - **Current Issue**: Entity API is basic global functions, not OOP
- **LuaRuntimeSystem.cpp/h**: Script lifecycle manager (awake/start/update)
- **ScriptAssetService.cpp/h**: Script asset loading and metadata
##### Physics
- **PhysicsSystem.cpp/h**: Box2D (2D) and Jolt (3D) integration
  - **Status**: Stub implementation, not fully wired
##### Assets
- **AssetWatcher.cpp/h**: File system watcher (efsw) for hot-reload
##### UI
- **Overlay.cpp/h**: ImGui docking layout, panel registration
---
### 2. **Editor** (Development Environment)
**Location:** `Editor/`
#### Components:
- **EditorLayer.cpp/h**: Main editor orchestration
- **Panels/**: Modular UI panels
  - **HierarchyPanel**: Scene entity tree
  - **InspectorPanel**: Component editing
  - **ScenePanel**: Viewport rendering
  - **AssetsPanel**: Asset browser
  - **ConsolePanel**: Log viewer
  - **MainMenuPanel**: File/Edit/View menus
#### Architecture Pattern:
- Panel system uses virtual `IEditorPanel` interface
- Context-based dependency injection via `PanelContext`
- Dockable ImGui layout with save/restore
---
### 3. **Runtime** (Standalone Launcher)
**Location:** `Runtime/`
- Minimal `main.cpp` that creates `Application` with `enableEditor = false`
- Intended to load `.artifactpkg` bundles for shipped games
- **Status**: Currently incomplete
---
### 4. **Plugins** (Extension System)
**Location:** `Plugins/`
#### Structure:
- **Core/PluginAPI.h**: Plugin interface definition
- **SplineShooter/**: Example gameplay plugin
  - `SplineShooterPlugin.cpp`: Spline-based shooting mechanics
#### Design:
- Simple C++ API: `Plugin::onRegister(PluginContext)`
- **Limitation**: No dynamic loading (.dll/.so), plugins are statically linked
---
### 5. **Tools** (Asset Pipeline)
**Location:** `Tools/`
- **AssetPipeline/PipelineMain.cpp**: Cook-on-import CLI tool
  - Texture compression (BC7/ASTC) - *stubbed*
  - Model import (Assimp) - *stubbed*
  - Shader validation (GLSL → SPIR-V) - *stubbed*
- **Packaging/**: Bundle creator for runtime - *incomplete*
---
### 6. **Games** (Project Workspace)
**Location:** `Games/SplineShooterLab/`
- Example game project structure
- `GameModule.cpp`: Game-specific initialization
- `Scripts/main.lua`: Lua entry point
---
## Technology Stack
### Core Technologies
| Component | Library | Version | Purpose |
|-----------|---------|---------|---------|
| **Graphics** | OpenGL | 3.3 Core | Cross-platform rendering |
| **Windowing** | GLFW3 | Latest | Window/input management |
| **GL Loader** | GLAD | - | OpenGL function loading |
| **Math** | GLM | Latest | Vector/matrix math |
| **ECS** | EnTT | Latest | Entity-Component-System |
| **Scripting** | LuaJIT | 2.1 | High-performance Lua VM |
| **Lua Binding** | Sol2 | Latest | Modern C++/Lua bridge |
| **UI** | ImGui | Docking | Editor interface |
| **UI Gizmos** | ImGuizmo | Latest | 3D manipulation widgets |
| **Logging** | spdlog | Latest | Fast structured logging |
| **Profiling** | Tracy | Latest | Frame profiler |
### Physics
| Component | Library | Purpose |
|-----------|---------|---------|
| **2D Physics** | Box2D | Rigid body 2D simulation |
| **3D Physics** | Jolt | High-performance 3D physics |
### Asset Management
| Component | Library | Purpose |
|-----------|---------|---------|
| **File Watch** | efsw | Hot-reload monitoring |
| **Model Import** | Assimp | 3D model loading |
| **Image Loading** | stb_image | Texture loading |
### Build System
- **CMake 3.24+**: Multi-platform build configuration
- **vcpkg**: Dependency management
- **Ninja**: Fast build backend
- **Catch2**: Unit testing framework
---
## Current Strengths
### ✅ Excellent Foundations
1. **Modern C++23**: Clean, type-safe codebase
2. **EnTT ECS**: High-performance, cache-friendly architecture
3. **Hot Reload**: Scripts and assets reload without restart
4. **Tracy Integration**: Built-in profiling from day one
5. **Cross-Platform**: Windows, Linux, macOS support
6. **ImGui Editor**: Mature, dockable UI system
### ✅ Developer Experience
- **Fast Iteration**: Edit-play cycle is very fast
- **Modular Design**: Clear separation of concerns
- **CMake Presets**: Easy configuration management
- **Compile Commands**: IDE integration (CLion, VSCode)
---
## Critical Issues & Gaps
### ❌ Major Architectural Gaps
#### 1. **No C++ Camera System**
**Current State:**
- Camera is implemented entirely in Lua (`CameraController.lua`)
- Transform manipulation happens through global Lua functions
- No projection matrix management
- No viewport/frustum culling
**Impact:** 
- Performance bottleneck for core functionality
- Cannot leverage C++ SIMD optimizations
- Difficult to integrate with rendering pipeline
#### 2. **Minimal Rendering Architecture**
**Current State:**
- `Renderer` class is just a framebuffer manager
- No mesh/material system
- No render queue or command buffers
- Skybox is hardcoded gradient
**Impact:**
- Cannot render actual 3D models
- No batching or instancing
- No PBR or advanced materials
#### 3. **Physics Not Integrated**
**Current State:**
- Box2D/Jolt are linked but not exposed to Lua
- No physics components
- No collision detection
#### 4. **Scripting Over-Reliance**
**Current State:**
- Core gameplay (camera, movement) is in Lua
- No C++ gameplay framework
**Philosophy Issue:**
- Lua should be for **game logic**, not **engine features**
- C++ should provide **performance-critical systems**
#### 5. **Asset Pipeline is Stubbed**
**Current State:**
- `PipelineMain.cpp` just creates directories
- No actual texture compression
- No model cooking
#### 6. **No Audio System**
- Zero audio implementation
---
## Recommendations Summary
### Immediate Priorities (Phase 1)
1. ✅ **Implement C++ Camera System** (see redesign docs)
2. ✅ **Build Rendering Architecture** (Mesh, Material, Shader systems)
3. ✅ **Integrate Physics** (expose Box2D/Jolt to gameplay layer)
4. ✅ **Define Lua/C++ Boundary** (clear separation of concerns)
### Medium-Term (Phase 2)
5. ✅ **Asset Pipeline Implementation** (texture compression, model import)
6. ✅ **Audio System** (OpenAL Soft or miniaudio)
7. ✅ **Better Plugin System** (dynamic loading, hot-reload)
### Long-Term (Phase 3)
8. ✅ **Runtime Packaging** (bundle creator, encryption)
9. ✅ **Networking Layer** (multiplayer foundation)
10. ✅ **Advanced Rendering** (PBR, shadows, post-processing)
---
## Next Steps
See:
- `REDESIGN_PROPOSAL.md` - Detailed architectural changes
- `CPP_CAMERA_IMPLEMENTATION.md` - Camera implementation guide
- `LUA_CPP_BOUNDARIES.md` - Scripting philosophy
- `IMPLEMENTATION_ROADMAP.md` - 6-month timeline
