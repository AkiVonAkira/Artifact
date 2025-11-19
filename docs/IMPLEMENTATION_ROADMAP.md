# Artifact Engine - Implementation Roadmap

**Vision:** Transform Artifact from a prototype engine to a production-ready game development platform with best-in-class C++ systems and flexible Lua scripting.

**Timeline:** 6 months (Phases 1-3)  
**Current Version:** 0.1.0  
**Target Version:** 0.3.0

---

## Current State Assessment

### ✅ What's Working Well
- **Core Architecture**: Clean ECS with EnTT
- **Editor Foundation**: ImGui docking, panels system
- **Hot Reload**: Lua scripts and assets reload seamlessly
- **Build System**: CMake + vcpkg + presets work well
- **Profiling**: Tracy integration ready for optimization
- **Cross-Platform**: Windows builds confirmed, Linux/macOS likely work

### ❌ Critical Gaps
- **No C++ Camera**: Core feature in Lua (performance bottleneck)
- **Minimal Rendering**: No mesh/material/shader system
- **Physics Not Integrated**: Box2D/Jolt linked but unused
- **Over-Reliant on Lua**: Core systems should be C++
- **Asset Pipeline Stubbed**: No actual texture/model cooking
- **No Audio**: Zero sound implementation
- **Documentation**: Limited API reference

---

## Design Principles (Going Forward)

1. **Performance First**: Hot paths in C++, profiled with Tracy
2. **Lua for Logic**: Game-specific code stays flexible
3. **Data-Driven**: Components over inheritance
4. **Editor-Centric**: All features accessible via UI
5. **Testable**: Unit tests for all systems
6. **Cross-Platform**: Windows/Linux/macOS parity

---

## Phase 1: Core Systems (Weeks 1-4)

**Goal:** Establish performance-critical C++ systems to replace Lua implementations.

### Week 1: Camera System ⭐ HIGHEST PRIORITY
- [ ] Implement `Camera` class (C++)
  - Perspective and orthographic projection
  - View/projection matrices with dirty flags
  - Frustum culling (plane extraction)
  - Screen↔World coordinate conversion
- [ ] Add `CameraComponent` to ECS
- [ ] Create `CameraSystem` for entity queries
- [ ] Integrate with `Renderer::renderScene()`
- [ ] Write unit tests (`test_camera.cpp`)
- [ ] Add Lua bindings (optional prototyping)
- [ ] Update Inspector panel for camera editing
- [ ] Deprecate `CameraController.lua`
- [ ] Document in `LUA_API_REFERENCE.md`

**Deliverable:** Camera updates in <0.01ms (Tracy), frustum culling functional

---

### Week 2: Input System
- [ ] Implement `InputSystem` class
  - Keyboard state tracking (down, pressed, released)
  - Mouse position, delta, scroll
  - Gamepad support (GLFW gamepad API)
- [ ] Add input action mapping (e.g., "Jump" → Space)
- [ ] Replace Lua global input functions
- [ ] Add Lua wrapper for Input API
- [ ] Test with multiple input devices

**Deliverable:** Input queries in <0.001ms, Lua API parity

---

### Week 3: Rendering Architecture (Part 1)
- [ ] **Mesh System**
  - `Vertex` struct (position, normal, UV, tangent)
  - `Mesh` class (VAO/VBO/EBO management)
  - Bounding box calculation
  - Basic primitives (cube, sphere, plane)
- [ ] **Shader System**
  - `Shader` class (compile, link, uniform setters)
  - Shader hot-reload (watch GLSL files)
  - Basic shaders (unlit, lit, PBR stub)
- [ ] Add `MeshRendererComponent`
- [ ] Test with simple cube rendering

**Deliverable:** Render 1000 cubes at 60 FPS

---

### Week 4: Rendering Architecture (Part 2)
- [ ] **Material System**
  - `Material` class (shader + properties)
  - Texture binding (albedo, normal, metallic/roughness)
  - Material editor in Inspector panel
- [ ] **Render Queue**
  - Opaque/transparent separation
  - Depth sorting for transparency
  - Frustum culling integration
- [ ] **Scene Rendering**
  - `Renderer::renderScene()` iterates entities
  - Batch by material (reduce state changes)
  - Multi-camera support

**Deliverable:** 10,000 entities with frustum culling at 60 FPS

---

## Phase 2: Physics & Gameplay (Weeks 5-8)

**Goal:** Integrate physics engines and build gameplay framework.

### Week 5: Physics Components
- [ ] Define physics components:
  - `RigidBody2DComponent` (Box2D)
  - `BoxCollider2DComponent`, `CircleCollider2DComponent`
  - `RigidBody3DComponent` (Jolt)
  - `BoxCollider3DComponent`, `SphereCollider3DComponent`
- [ ] Update `PhysicsSystem` to use Box2D API
  - Create `b2World`, `b2Body` on component add
  - Sync transforms: Physics → ECS
  - Handle collision callbacks
- [ ] Write physics unit tests

**Deliverable:** 2D physics fully functional

---

### Week 6: Physics Integration (3D)
- [ ] Integrate Jolt Physics
  - Setup `JPH::PhysicsSystem`
  - Create rigid bodies, shapes, constraints
  - Sync transforms: Physics → ECS
- [ ] Add Lua physics API:
  - `Physics.ApplyForce(x, y, z)`
  - `Physics.Raycast(origin, direction, maxDist)`
  - `Physics.OverlapSphere(center, radius)`
- [ ] Physics debugging visualizer (draw colliders)

**Deliverable:** 3D physics functional, Lua API documented

---

### Week 7: Component Registry
- [ ] Implement `ComponentRegistry` pattern
  - `registerComponent<T>("Name")`
  - Dynamic component creation from string
  - Reflection-like queries (list all components)
- [ ] Update Inspector panel:
  - "Add Component" dropdown (all registered types)
  - Serialize/deserialize components to JSON
- [ ] Add common gameplay components:
  - `HealthComponent`
  - `CharacterControllerComponent` (prototype)
  - `InventoryComponent` (stub)

**Deliverable:** Add any component via editor UI

---

### Week 8: Scene Serialization
- [ ] Implement scene save/load (JSON format)
  - Entity hierarchy
  - All component data
  - Prefab references
- [ ] Scene file format versioning
- [ ] Auto-save on Play mode entry
- [ ] Scene diff viewer (for version control)

**Deliverable:** Save/load complex scenes without data loss

---

## Phase 3: Asset Pipeline & Tools (Weeks 9-12)

**Goal:** Complete asset cooking pipeline for production builds.

### Week 9: Texture Pipeline
- [ ] Texture compression:
  - BC7 (Windows/Linux) via DirectXTex or compressonator
  - ASTC (Mobile) via astcenc
- [ ] Mipmap generation
- [ ] Asset metadata files (`.meta`)
- [ ] Texture import settings (compression, filtering, wrap)
- [ ] Batch asset cooking CLI

**Deliverable:** 4K texture → 10MB compressed (BC7)

---

### Week 10: Model Pipeline
- [ ] Assimp model import:
  - FBX, glTF, OBJ support
  - Mesh optimization (vertex cache, index buffer)
  - Material extraction
- [ ] Custom mesh format (`.artmesh`)
  - Binary, fast to load
  - Version tagging
- [ ] Model preview in Asset panel

**Deliverable:** Import Unity FBX files successfully

---

### Week 11: Shader Pipeline
- [ ] GLSL → SPIR-V compilation (glslang)
- [ ] Shader includes (`#include "common.glsl"`)
- [ ] Shader variants (e.g., `#define USE_NORMAL_MAP`)
- [ ] Shader graph (ImNodes) - prototype

**Deliverable:** Hot-reload shaders without restart

---

### Week 12: Audio System
- [ ] Integrate miniaudio (or OpenAL Soft)
- [ ] Add `AudioSourceComponent`
  - 2D/3D audio
  - Volume, pitch, loop settings
- [ ] Audio mixer (volume groups)
- [ ] Lua audio API:
  - `Audio.Play("sound.wav")`
  - `Audio.SetVolume("Master", 0.5)`
- [ ] Audio debugging (visualize 3D sound sources)

**Deliverable:** Play 100+ sounds simultaneously at 60 FPS

---

## Phase 4: Advanced Features (Months 4-6)

### Month 4: Animation System
- [ ] Skeletal animation
  - Bone hierarchy
  - Skinning (GPU skinning)
  - Animation clips (import from FBX)
- [ ] Animation state machine
  - Blend trees
  - Transitions with conditions
- [ ] Animator component
- [ ] Animation editor panel (timeline)

---

### Month 5: Advanced Rendering
- [ ] PBR shading (Cook-Torrance)
- [ ] Image-based lighting (IBL)
  - Skybox → irradiance map
  - Pre-filtered environment map
- [ ] Shadow mapping
  - Directional light shadows
  - Cascaded shadow maps (CSM)
- [ ] Post-processing
  - Bloom, tonemapping, color grading
  - FXAA/SMAA anti-aliasing

---

### Month 6: Editor Polish
- [ ] Undo/Redo system (command pattern)
- [ ] Prefab system overhaul
  - Prefab variants
  - Nested prefabs
- [ ] Multi-scene editing
- [ ] Build pipeline (package `.artifactpkg`)
- [ ] Publish to itch.io/Steam (export)

---

## Testing Strategy

### Unit Tests (Catch2)
- **Target Coverage:** 70%+
- **Key Systems:**
  - Camera (view/projection matrices)
  - Physics (collision detection)
  - Asset loading (mesh/texture parsing)
  - Scene serialization (JSON round-trip)

### Integration Tests
- **Editor Workflow Tests:**
  - Create entity → Add components → Save scene → Load scene
  - Play mode → Stop → Verify state restored
- **Performance Tests:**
  - 10,000 entities at 60 FPS
  - 1,000 draw calls per frame

### Continuous Integration (GitHub Actions)
```yaml
# .github/workflows/ci.yml
name: Build & Test

on: [push, pull_request]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - uses: lukka/run-vcpkg@v11
      - run: cmake --preset editor-release
      - run: cmake --build build
      - run: ctest --output-on-failure
      
  build-linux:
    runs-on: ubuntu-latest
    steps:
      # Similar to Windows
```

---

## Documentation Plan

### For Developers (Internal)
- [x] `ARCHITECTURE_ANALYSIS.md` - Current state analysis
- [x] `REDESIGN_PROPOSAL.md` - Architectural changes
- [x] `CPP_CAMERA_IMPLEMENTATION.md` - Camera system guide
- [x] `LUA_CPP_BOUNDARIES.md` - Lua/C++ philosophy
- [x] `PACKAGE_MANAGER_COMPARISON.md` - Dependency management
- [ ] `RENDERING_ARCHITECTURE.md` - Graphics pipeline
- [ ] `PHYSICS_INTEGRATION.md` - Box2D/Jolt usage
- [ ] `ASSET_PIPELINE.md` - Texture/model cooking

### For Users (External)
- [ ] `API_REFERENCE.md` - Complete Lua API docs
- [ ] `GETTING_STARTED.md` - Tutorial (first game in 30 min)
- [ ] `COMPONENT_REFERENCE.md` - All built-in components
- [ ] `EDITOR_MANUAL.md` - UI guide with screenshots
- [ ] Video Tutorials (YouTube):
  - [ ] "Artifact Engine Overview" (10 min)
  - [ ] "Building a Platformer" (30 min series)
  - [ ] "Advanced Physics" (15 min)

---

## Performance Targets

### Frame Time Budget (60 FPS = 16.67ms)
| System | Budget | Current | Target |
|--------|--------|---------|--------|
| **Rendering** | 8ms | ??? | 6ms |
| **Physics** | 2ms | N/A | 2ms |
| **Scripting** | 3ms | ??? | 2ms |
| **Editor UI** | 2ms | ??? | 2ms |
| **Other** | 1.67ms | ??? | 1ms |

### Tracy Profiling Milestones
- [ ] Baseline current performance (Week 1)
- [ ] Camera system <0.01ms (Week 1)
- [ ] Frustum culling 10k entities <0.5ms (Week 4)
- [ ] Full frame render <6ms (Week 8)
- [ ] Physics update <2ms (Week 6)

---

## Risk Management

### Technical Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| **Physics integration breaks existing code** | Medium | High | Incremental integration, feature flags |
| **Rendering refactor causes visual bugs** | High | Medium | Side-by-side comparison mode |
| **Asset pipeline fails on edge cases** | High | Medium | Extensive test suite (100+ assets) |
| **Performance targets not met** | Medium | High | Early profiling, SIMD optimizations |
| **Cross-platform issues (macOS/Linux)** | Medium | Medium | CI testing, community feedback |

### Schedule Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| **Underestimated complexity** | High | High | Buffer weeks in schedule (20% slack) |
| **Scope creep** | Medium | High | Strict feature freeze after Phase 3 |
| **Dependency issues (vcpkg)** | Low | Medium | Binary caching, Docker CI |

---

## Success Metrics

### Quantitative
- ✅ **Build Time:** <2 min (incremental), <5 min (clean with cache)
- ✅ **Test Coverage:** 70%+
- ✅ **Frame Rate:** 60 FPS with 10,000 entities
- ✅ **Memory Usage:** <500 MB for editor (empty scene)
- ✅ **Startup Time:** <3 seconds (editor launch)

### Qualitative
- ✅ **Developer Experience:** Can create a simple game in <1 hour
- ✅ **Stability:** Zero crashes during normal usage
- ✅ **Documentation:** Complete API reference, 3+ tutorials
- ✅ **Community:** 10+ external contributors (if open source)

---

## Post-Launch (v0.3.0+)

### Version 0.4.0 (Months 7-9)
- Networking layer (replication, RPC)
- Visual scripting (Blueprint-like)
- Terrain system (heightmaps, LOD)
- Particle system (GPU-based)

### Version 0.5.0 (Months 10-12)
- VR support (OpenXR)
- Mobile build targets (Android/iOS)
- Advanced AI (behavior trees, navmesh)
- Plugin marketplace (community assets)

### Version 1.0.0 (Year 2)
- **Production-Ready**
  - Full documentation
  - Shipped games using Artifact
  - Commercial support option
  - Asset store integration

---

## Team & Responsibilities

### Suggested Roles (if expanding team)
- **Lead Engine Programmer** (You): Architecture, core systems
- **Graphics Programmer**: Rendering, shaders, optimization
- **Gameplay Programmer**: Physics, animation, gameplay framework
- **Tools Programmer**: Asset pipeline, editor features
- **Technical Artist**: Shaders, materials, VFX
- **QA/Tester**: Testing, CI/CD, performance validation

---

## Community Engagement (if Open Source)

### Communication Channels
- **GitHub Discussions**: Feature requests, Q&A
- **Discord Server**: Real-time chat, support
- **YouTube Channel**: Tutorials, devlogs
- **Blog/Dev Diary**: Monthly progress updates

### Contribution Guidelines
- `CONTRIBUTING.md` with coding standards
- Issue templates (bug, feature, documentation)
- PR review process (2 approvals required)

---

## Budget Considerations (if Commercial)

### Tooling Costs
- **GitHub Pro**: $4/month (private repos, CI minutes)
- **JetBrains CLion**: $199/year (or free for open source)
- **RenderDoc**: Free
- **Tracy**: Free

### Optional Services
- **Sentry (Crash Reporting)**: $26/month
- **Codecov (Coverage)**: Free for open source
- **DigitalOcean (CI Runners)**: $5/month

**Total:** ~$30-50/month

---

## Conclusion

**This roadmap transforms Artifact from a prototype to a production-ready engine in 6 months.**

### Key Takeaways:
1. **Phase 1 (Weeks 1-4):** C++ camera, input, rendering foundation
2. **Phase 2 (Weeks 5-8):** Physics integration, component system
3. **Phase 3 (Weeks 9-12):** Asset pipeline, audio, tools
4. **Post-Launch:** Animation, advanced rendering, networking

### Next Immediate Actions:
1. ✅ Review all documentation (this file + 4 others)
2. ✅ Set up project board (GitHub Projects or Trello)
3. ✅ Create Week 1 tasks (Camera system breakdown)
4. ✅ Enable vcpkg binary caching (5 min)
5. ✅ Write first unit test (`test_camera.cpp`)

### Long-Term Vision:
**Artifact becomes the go-to engine for:**
- Indie developers seeking C++ performance + Lua flexibility
- Prototyping game mechanics rapidly
- Educational projects (clean codebase for learning)
- 2D/3D games with excellent tooling

---

**Let's build something amazing! 🚀**

---

## Feature Readiness Snapshot (Nov 2025)

| Goal | Current Status | Next Steps |
| --- | --- | --- |
| Editor UI framework, entity management, and property editing (name/model/texture/transform) | ✅ Docked ImGui editor with Hierarchy/Inspector panels. Entities can be created, selected, renamed, scripted, and manipulated for transforms. Model/texture authoring UI not yet wired. | Hook Mesh/Texture components into Inspector once rendering subsystem lands. |
| Rendering all entities with mesh/material data | ⚠️ Renderer currently draws skybox only; mesh/material system pending (Phase 1 Week 3+). | Implement Mesh/MeshRenderer components, material editor, and draw loop batching. |
| OBJ loader + MeshManager cache | ❌ No asset importer yet; roadmap places mesh pipeline in Phase 3 (Week 10). | Integrate Assimp import path, add caching layer per Mesh GUID. |
| Game engine architecture doc | ✅ `docs/ARCHITECTURE_ANALYSIS.md` and `docs/REDESIGN_PROPOSAL.md` describe the current and future architecture. | Keep updated alongside subsystem milestones. |
| Message classes + queue | ⚠️ Messaging not implemented; Lua events suffice today. | Decide on C++ event bus interface, serialize to scripting bridge. |
| Memory checking utilities | ❌ No dedicated leak/check framework documented. | Evaluate Tracy counters or integrate CRT/Address Sanitizer toggles. |
| Mesh serialization | ❌ Awaiting mesh system; serialization format undefined. | Define `.artmesh` schema when MeshManager lands. |
| Thread & mutex abstractions | ⚠️ Engine relies on std::thread/mutex indirectly via subsystems; no custom wrappers. | Only implement if cross-platform control is required. |
| Camera class + UI configuration | ✅ `Rendering::Camera` in EngineCore with Inspector editing and Editor fly camera. Runtime multi-camera switch pending. | Add runtime camera toggles and toolbar shortcuts. |
| Texture loader (stb/other) | ⚠️ Placeholder; actual texture ingestion deferred to asset pipeline. | Choose stb_image for MVP, integrate into MeshManager/Material system. |
| Normals / UVs / triangulation support | ❌ Absent until mesh import/render is in place. | Land with OBJ/glTF ingestion. |
| Iterative resource loading | ❌ No streaming/resource manager yet. | Design resource jobs once asset pipeline implemented. |
| Camera manager + WASD controls | ⚠️ Editor camera supports WASD+QE, but runtime camera manager not implemented. | Extend CameraSystem to allow switching and keybindings in play mode. |
| Custom PNG loader (zlib) | ❌ Not started. | Evaluate stb vs. custom PNG; defer until texture system needed. |
| Scene/level serialization (entities + metadata) | ❌ Not in place; roadmap Phase 2 Week 8. | Implement JSON/protobuf scene save/load tied to ComponentRegistry. |

**Takeaway:** Editor UX, scripting, and camera foundations are in place. Rendering, asset importers, messaging, and serialization remain ahead per the roadmap. Keep executing Phase 1 milestones before tackling stretch goals like custom PNG loaders or iterative streaming.
