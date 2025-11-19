# Quick Start: Implementing the C++ Camera System

**Goal:** Replace Lua-based camera with performance-critical C++ implementation  
**Time:** 1 week  
**Difficulty:** Intermediate

---

## Prerequisites

- [ ] Read [ARCHITECTURE_ANALYSIS.md](ARCHITECTURE_ANALYSIS.md)
- [ ] Skim [REDESIGN_PROPOSAL.md](REDESIGN_PROPOSAL.md)
- [ ] Understand [LUA_CPP_BOUNDARIES.md](LUA_CPP_BOUNDARIES.md) philosophy

---

## Day 1: Core Camera Class

### 1. Create Header File

```bash
# In your terminal
cd C:\Projects\Unsorted\Artifact
mkdir -p EngineCore\include\Artifact\Rendering
```

**File:** `EngineCore\include\Artifact\Rendering\Camera.h`

Copy the complete Camera class from [CPP_CAMERA_IMPLEMENTATION.md](CPP_CAMERA_IMPLEMENTATION.md#step-1-core-camera-class)

### 2. Create Implementation File

**File:** `EngineCore\Rendering\Camera.cpp`

Copy the implementation from the same section.

### 3. Update CMakeLists.txt

**File:** `EngineCore\CMakeLists.txt`

```cmake
set(ENGINECORE_HEADERS
    # ...existing headers...
    include/Artifact/Rendering/Camera.h
)

set(ENGINECORE_SOURCES
    # ...existing sources...
    Rendering/Camera.cpp
)
```

### 4. Build and Test

```bash
cmake --build build/Windows/editor-debug
```

**Expected:** Zero compilation errors

---

## Day 2: ECS Component

### 1. Update Components Header

**File:** `EngineCore\include\Artifact\Scene\Components.h`

Add at the top:
```cpp
#include "Artifact/Rendering/Camera.h"
```

Add the component:
```cpp
namespace Artifact::Scene {

struct CameraComponent {
    Rendering::Camera camera;
    bool isPrimary{true};
    int renderOrder{0};
    uint32_t cullingMask{0xFFFFFFFF};
};

} // namespace Artifact::Scene
```

### 2. Create Camera System

**File:** `EngineCore\include\Artifact\Rendering\CameraSystem.h`

Copy from [CPP_CAMERA_IMPLEMENTATION.md](CPP_CAMERA_IMPLEMENTATION.md#step-3-camera-system)

**File:** `EngineCore\Rendering\CameraSystem.cpp`

Copy implementation from the same section.

### 3. Update CMakeLists.txt Again

```cmake
set(ENGINECORE_HEADERS
    # ...existing...
    include/Artifact/Rendering/CameraSystem.h
)

set(ENGINECORE_SOURCES
    # ...existing...
    Rendering/CameraSystem.cpp
)
```

### 4. Test Compilation

```bash
cmake --build build/Windows/editor-debug
```

---

## Day 3: Integration with Renderer

### 1. Update Renderer Header

**File:** `EngineCore\include\Artifact\Rendering\Renderer.h`

Add forward declaration:
```cpp
namespace Artifact::Rendering {
    class Camera;
    class CameraSystem;
}
```

Add method:
```cpp
class Renderer {
public:
    // ...existing methods...
    void renderScene(entt::registry& registry); // NEW
    
private:
    CameraSystem m_cameraSystem; // NEW
};
```

### 2. Update Renderer Implementation

**File:** `EngineCore\Rendering\Renderer.cpp`

Add includes:
```cpp
#include "Artifact/Rendering/CameraSystem.h"
#include "Artifact/Scene/Components.h"
```

Add method (basic version for now):
```cpp
void Renderer::renderScene(entt::registry& registry) {
    Camera* primaryCam = CameraSystem::getPrimaryCamera(registry);
    if (!primaryCam) {
        spdlog::warn("No primary camera in scene");
        return;
    }
    
    // TODO: Actual rendering logic will be added later
    // For now, just verify camera is accessible
    spdlog::info("Rendering from camera at position: ({}, {}, {})",
        primaryCam->position().x,
        primaryCam->position().y,
        primaryCam->position().z);
}
```

### 3. Test

Build and run the editor. Check console for log messages.

---

## Day 4: Unit Tests

### 1. Create Test File

**File:** `tests\test_camera.cpp`

Copy complete tests from [CPP_CAMERA_IMPLEMENTATION.md](CPP_CAMERA_IMPLEMENTATION.md#step-7-testing)

### 2. Update Tests CMakeLists

**File:** `tests\CMakeLists.txt`

Add:
```cmake
target_sources(artifact_tests PRIVATE
    # ...existing...
    test_camera.cpp
)
```

### 3. Run Tests

```bash
cd build/Windows/editor-debug
ctest --output-on-failure
```

**Expected:** All camera tests pass ✅

---

## Day 5: Lua Bindings

### 1. Update LuaBindings Header

**File:** `EngineCore\include\Artifact\Scripting\LuaBindings.h`

Add method declaration:
```cpp
class LuaBindings {
public:
    // ...existing...
    void registerCameraBindings(); // NEW
};
```

### 2. Implement Lua Bindings

**File:** `EngineCore\Scripting\LuaBindings.cpp`

Copy implementation from [CPP_CAMERA_IMPLEMENTATION.md](CPP_CAMERA_IMPLEMENTATION.md#step-5-lua-bindings-optional)

### 3. Register in LuaContext

**File:** `EngineCore\Core\Application.cpp`

In `Application::Application()`, add:
```cpp
m_luaBindings->registerCameraBindings();
```

### 4. Test Lua API

Create test script: `Assets\Raw\Scripts\TestCamera.lua`

```lua
local TestCamera = {}

function TestCamera:start()
    print("Testing Camera API from Lua")
    Camera.SetPosition(0, 10, 20)
    Camera.LookAt(0, 0, 0)
    
    local pos = Camera.GetPosition()
    print(string.format("Camera position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z))
end

return TestCamera
```

Run editor, attach script to an entity with CameraComponent, and verify output.

---

## Day 6: Editor Integration

### 1. Update InspectorPanel

**File:** `Editor\Panels\InspectorPanel.cpp`

Add method to draw camera component (see [CPP_CAMERA_IMPLEMENTATION.md](CPP_CAMERA_IMPLEMENTATION.md#step-6-editor-integration))

### 2. Add Camera Creation Helper

**File:** `EngineCore\Scene\SceneService.cpp`

Update `createDefaultCamera()`:
```cpp
entt::entity SceneService::createDefaultCamera() {
    auto entity = createEntity("Camera");
    
    auto& camComp = m_scene.registry.emplace<Scene::CameraComponent>(entity);
    camComp.camera.setPosition({0.0f, 10.0f, 20.0f});
    camComp.camera.lookAt({0.0f, 0.0f, 0.0f});
    camComp.camera.setPerspective(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    
    return entity;
}
```

### 3. Test in Editor

1. Launch editor
2. Create entity
3. Add "Camera" component via Inspector
4. Modify FOV, position, rotation
5. Verify changes in viewport

---

## Day 7: Performance Profiling & Cleanup

### 1. Add Tracy Zones

**File:** `EngineCore\Rendering\Renderer.cpp`

```cpp
void Renderer::renderScene(entt::registry& registry) {
    ARTIFACT_ZONE_NAMED("Renderer::renderScene");
    
    {
        ARTIFACT_ZONE_NAMED("Get Primary Camera");
        Camera* primaryCam = CameraSystem::getPrimaryCamera(registry);
        // ...
    }
}
```

### 2. Profile with Tracy

1. Build with Tracy enabled: `cmake --preset editor-debug`
2. Run `tracy-profiler.exe` (download from [Tracy releases](https://github.com/wolfpld/tracy/releases))
3. Connect to running Artifact Editor
4. Verify camera operations take <0.01ms

### 3. Deprecate Lua Camera

```bash
# Move old Lua camera to deprecated folder
mkdir -p Assets\Raw\Scripts\Deprecated
move Assets\Raw\Scripts\CameraController.lua Assets\Raw\Scripts\Deprecated\
```

Create `Assets\Raw\Scripts\Deprecated\README.txt`:
```
These scripts are deprecated and replaced by C++ implementations.
Kept for reference only.
```

### 4. Update Documentation

Create `docs\CHANGELOG.md`:
```markdown
# Changelog

## [0.2.0] - 2025-11-XX

### Added
- C++ Camera system with frustum culling
- CameraComponent and CameraSystem
- Lua Camera API for prototyping
- Unit tests for camera (test_camera.cpp)

### Changed
- Camera is now implemented in C++ instead of Lua
- Performance improved: Camera updates now <0.01ms (was ~1ms)

### Deprecated
- CameraController.lua (moved to Assets/Raw/Scripts/Deprecated/)

### Removed
- None (backwards compatibility maintained via Lua API)
```

---

## Success Checklist

By the end of Week 1, you should have:

- [x] `Camera` class implemented in C++
- [x] `CameraComponent` added to ECS
- [x] `CameraSystem` for entity queries
- [x] Integration with `Renderer::renderScene()`
- [x] Unit tests passing (>80% coverage)
- [x] Lua bindings functional
- [x] Inspector panel can edit camera properties
- [x] Tracy profiling shows <0.01ms update time
- [x] `CameraController.lua` deprecated
- [x] Documentation updated

---

## Common Issues & Solutions

### Issue: "Camera.h not found"
**Solution:** Make sure `EngineCore\include` is in target's include directories:
```cmake
target_include_directories(artifact_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
```

### Issue: Linker errors with Camera symbols
**Solution:** Verify `Camera.cpp` is in `ENGINECORE_SOURCES` in CMakeLists.txt

### Issue: Tests fail with "No primary camera"
**Solution:** Create a camera entity in test setup:
```cpp
TEST_CASE("Camera System - Get Primary", "[CameraSystem]") {
    entt::registry registry;
    auto entity = registry.create();
    auto& camComp = registry.emplace<Scene::CameraComponent>(entity);
    camComp.isPrimary = true;
    
    Camera* cam = CameraSystem::getPrimaryCamera(registry);
    REQUIRE(cam != nullptr);
}
```

### Issue: Lua bindings crash
**Solution:** Ensure `m_registry` and `m_activeEntity` are set before calling Lua functions:
```cpp
// In EditorLayer or wherever Lua is invoked
m_luaBindings->setRegistry(&m_sceneService->context().registry);
m_luaBindings->setActiveEntity(selectedEntity);
```

---

## Next Steps (Week 2)

After completing the Camera system:

1. **Input System** (see [IMPLEMENTATION_ROADMAP.md](IMPLEMENTATION_ROADMAP.md#week-2-input-system))
2. **Rendering Architecture Part 1** (Mesh, Shader)
3. Continue following the roadmap

---

## Questions?

- Check [docs/README.md](README.md) for full documentation index
- Review [CPP_CAMERA_IMPLEMENTATION.md](CPP_CAMERA_IMPLEMENTATION.md) for detailed code
- See [LUA_CPP_BOUNDARIES.md](LUA_CPP_BOUNDARIES.md) for design philosophy

---

**Good luck! You're building something awesome! 🚀**


