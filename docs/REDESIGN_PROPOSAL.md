# Artifact Engine - Architecture Redesign Proposal

**Date:** November 19, 2025  
**Target Version:** 0.2.0

---

## Design Philosophy

### Core Principle: **C++ for Systems, Lua for Game Logic**

```
┌─────────────────────────────────────────────────┐
│              GAME LAYER (Lua)                   │
│  • Enemy AI behaviors                           │
│  • Quest logic                                  │
│  • UI interactions                              │
│  • Gameplay rules                               │
└────────────────┬────────────────────────────────┘
                 │ Lua API
┌────────────────┴────────────────────────────────┐
│         GAMEPLAY FRAMEWORK (C++)                │
│  • Character controllers                        │
│  • Inventory systems                            │
│  • Game state machines                          │
└────────────────┬────────────────────────────────┘
                 │ Component API
┌────────────────┴────────────────────────────────┐
│            ENGINE CORE (C++)                    │
│  • Camera systems                               │
│  • Physics simulation                           │
│  • Rendering pipeline                           │
│  • Asset management                             │
│  • Input handling                               │
└─────────────────────────────────────────────────┘
```

**Rationale:**
- **Performance**: Hot paths (rendering, physics) stay in C++
- **Safety**: Type-safe component system prevents runtime errors
- **Flexibility**: Lua provides rapid iteration for game-specific code
- **Best of Both Worlds**: C++ stability + Lua agility

---

## Proposed Module Architecture

### 1. New Camera System (C++)

**Location:** `EngineCore/include/Artifact/Rendering/Camera.h`

```cpp
namespace Artifact::Rendering {

enum class ProjectionType {
    Perspective,
    Orthographic
};

class Camera {
public:
    Camera(ProjectionType type = ProjectionType::Perspective);
    
    // Transform
    void setPosition(const glm::vec3& pos);
    void setRotation(const glm::quat& rot);
    void lookAt(const glm::vec3& target, const glm::vec3& up = {0,1,0});
    
    // Projection
    void setPerspective(float fov, float aspect, float near, float far);
    void setOrthographic(float left, float right, float bottom, float top, float near, float far);
    
    // Getters
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;
    glm::mat4 viewProjectionMatrix() const;
    glm::vec3 forward() const;
    glm::vec3 right() const;
    glm::vec3 up() const;
    
    // Frustum culling
    bool isVisible(const AABB& bounds) const;
    
private:
    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
    glm::quat m_rotation{1.0f, 0.0f, 0.0f, 0.0f}; // Identity quaternion
    
    ProjectionType m_projType;
    float m_fov{45.0f};
    float m_aspectRatio{16.0f/9.0f};
    float m_nearPlane{0.1f};
    float m_farPlane{1000.0f};
    
    // Orthographic params
    float m_orthoLeft{-10.0f}, m_orthoRight{10.0f};
    float m_orthoBottom{-10.0f}, m_orthoTop{10.0f};
    
    mutable glm::mat4 m_viewMatrix{1.0f};
    mutable glm::mat4 m_projectionMatrix{1.0f};
    mutable bool m_viewDirty{true};
    mutable bool m_projDirty{true};
};

} // namespace Artifact::Rendering
```

**Component Integration:**

```cpp
// EngineCore/include/Artifact/Scene/Components.h
namespace Artifact::Scene {

struct CameraComponent {
    Rendering::Camera camera;
    bool isPrimary{true};      // Main scene camera
    int renderOrder{0};        // For multi-camera setups
    uint32_t cullingMask{0xFF}; // Layer-based rendering
};

} // namespace Artifact::Scene
```

**System Implementation:**

```cpp
// EngineCore/Rendering/CameraSystem.h
namespace Artifact::Rendering {

class CameraSystem {
public:
    void update(entt::registry& registry);
    Camera* getPrimaryCamera(entt::registry& registry);
    void renderFromCamera(const Camera& cam, const Scene::SceneContext& scene);
    
private:
    void updateFrustumCulling(Camera& cam, entt::registry& registry);
};

} // namespace Artifact::Rendering
```

**Lua Bindings (Optional for Prototyping):**

```lua
-- Still allow Lua to control camera for prototyping
local cam = Entity.GetComponent("Camera")
cam:SetPosition(0, 10, 20)
cam:LookAt(0, 0, 0)
```

---

### 2. New Rendering Architecture

**Goal:** Replace the minimal `Renderer` with a proper graphics pipeline.

#### 2.1 Mesh System

```cpp
// EngineCore/include/Artifact/Rendering/Mesh.h
namespace Artifact::Rendering {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 tangent;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~Mesh();
    
    void bind() const;
    void draw() const;
    
    const AABB& bounds() const { return m_bounds; }
    
private:
    GLuint m_vao{0}, m_vbo{0}, m_ebo{0};
    uint32_t m_indexCount{0};
    AABB m_bounds;
};

using MeshHandle = std::shared_ptr<Mesh>;

} // namespace Artifact::Rendering
```

#### 2.2 Material System

```cpp
// EngineCore/include/Artifact/Rendering/Material.h
namespace Artifact::Rendering {

class Shader {
public:
    Shader(const std::string& vertSrc, const std::string& fragSrc);
    void use() const;
    void setUniform(const std::string& name, const glm::mat4& value);
    void setUniform(const std::string& name, const glm::vec3& value);
    // ... more uniform setters
    
private:
    GLuint m_programId{0};
    std::unordered_map<std::string, GLint> m_uniformCache;
};

struct MaterialProperties {
    glm::vec4 albedo{1.0f};
    float metallic{0.0f};
    float roughness{0.5f};
    float ao{1.0f};
    
    uint32_t albedoMap{0};
    uint32_t normalMap{0};
    uint32_t metallicRoughnessMap{0};
};

class Material {
public:
    Material(std::shared_ptr<Shader> shader);
    
    void bind() const;
    void setProperty(const std::string& name, const MaterialProperties& props);
    
private:
    std::shared_ptr<Shader> m_shader;
    MaterialProperties m_properties;
};

} // namespace Artifact::Rendering
```

#### 2.3 Render Queue

```cpp
// EngineCore/include/Artifact/Rendering/RenderQueue.h
namespace Artifact::Rendering {

struct RenderCommand {
    MeshHandle mesh;
    std::shared_ptr<Material> material;
    glm::mat4 transform;
    uint32_t layer{0};
    float distanceToCamera{0.0f}; // For depth sorting
};

class RenderQueue {
public:
    void submit(const RenderCommand& cmd);
    void sort(const Camera& camera);
    void execute() const;
    void clear();
    
private:
    std::vector<RenderCommand> m_opaqueQueue;
    std::vector<RenderCommand> m_transparentQueue;
};

} // namespace Artifact::Rendering
```

#### 2.4 Renderer Refactor

```cpp
// EngineCore/include/Artifact/Rendering/Renderer.h (Updated)
namespace Artifact::Rendering {

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    // Frame lifecycle
    void beginFrame(const FrameContext& ctx);
    void endFrame();
    
    // Scene rendering
    void renderScene(const Camera& camera, entt::registry& registry);
    
    // Viewport management
    void beginSceneViewport(int width, int height);
    void endSceneViewport();
    uint32_t sceneColorTexture() const;
    
    // Render queue access
    RenderQueue& queue() { return m_renderQueue; }
    
private:
    void initializeGlState();
    void ensureSceneFramebuffer(int width, int height);
    void cullAndBatch(const Camera& camera, entt::registry& registry);
    
    // Framebuffer
    unsigned int m_sceneFramebuffer{0};
    unsigned int m_sceneColor{0};
    unsigned int m_sceneDepth{0};
    int m_sceneWidth{0};
    int m_sceneHeight{0};
    
    // Rendering
    RenderQueue m_renderQueue;
    CameraSystem m_cameraSystem;
};

} // namespace Artifact::Rendering
```

**Component for Rendering:**

```cpp
// EngineCore/include/Artifact/Scene/Components.h
namespace Artifact::Scene {

struct MeshRendererComponent {
    Rendering::MeshHandle mesh;
    std::shared_ptr<Rendering::Material> material;
    bool castShadows{true};
    bool receiveShadows{true};
    uint32_t layer{0}; // For culling masks
};

} // namespace Artifact::Scene
```

---

### 3. Physics Integration

**Goal:** Expose Box2D/Jolt to gameplay layer with proper components.

#### 3.1 Physics Components

```cpp
// EngineCore/include/Artifact/Physics/Components.h
namespace Artifact::Physics {

enum class BodyType {
    Static,
    Kinematic,
    Dynamic
};

struct RigidBody2DComponent {
    BodyType type{BodyType::Dynamic};
    float mass{1.0f};
    float linearDamping{0.0f};
    float angularDamping{0.0f};
    bool fixedRotation{false};
    glm::vec2 velocity{0.0f};
    float angularVelocity{0.0f};
    
    // Internal
    void* box2dBody{nullptr}; // b2Body*
};

struct BoxCollider2DComponent {
    glm::vec2 size{1.0f, 1.0f};
    glm::vec2 offset{0.0f};
    float density{1.0f};
    float friction{0.3f};
    float restitution{0.0f};
    bool isTrigger{false};
};

struct RigidBody3DComponent {
    BodyType type{BodyType::Dynamic};
    float mass{1.0f};
    bool lockRotationX{false};
    bool lockRotationY{false};
    bool lockRotationZ{false};
    glm::vec3 velocity{0.0f};
    glm::vec3 angularVelocity{0.0f};
    
    // Internal
    void* joltBodyId{nullptr};
};

struct BoxCollider3DComponent {
    glm::vec3 size{1.0f};
    glm::vec3 offset{0.0f};
    float friction{0.5f};
    float restitution{0.0f};
    bool isTrigger{false};
};

} // namespace Artifact::Physics
```

#### 3.2 Physics System Refactor

```cpp
// EngineCore/include/Artifact/Physics/PhysicsSystem.h (Updated)
namespace Artifact::Physics {

class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();
    
    // Simulation
    void update2D(entt::registry& registry, float dt);
    void update3D(entt::registry& registry, float dt);
    
    // Queries
    std::vector<entt::entity> raycast2D(const glm::vec2& origin, const glm::vec2& direction, float maxDist);
    std::vector<entt::entity> overlapBox3D(const glm::vec3& center, const glm::vec3& halfExtents);
    
    // Force application
    void applyForce2D(entt::entity e, const glm::vec2& force);
    void applyImpulse3D(entt::entity e, const glm::vec3& impulse);
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    
    void syncTransformFromPhysics(entt::registry& registry);
    void syncTransformToPhysics(entt::registry& registry);
};

} // namespace Artifact::Physics
```

#### 3.3 Lua Bindings

```lua
-- Example: Add physics in Lua
Entity.AddComponent("RigidBody2D", {
    type = "Dynamic",
    mass = 10.0
})

Entity.AddComponent("BoxCollider2D", {
    size = {x = 2.0, y = 2.0}
})

Physics.ApplyForce(Entity.GetID(), {x = 0, y = 100})
```

---

### 4. Input System (C++)

**Goal:** Move input handling from Lua global to a proper C++ system.

```cpp
// EngineCore/include/Artifact/Input/InputSystem.h
namespace Artifact::Input {

enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2
};

class InputSystem {
public:
    InputSystem(GLFWwindow* window);
    
    // Keyboard
    bool isKeyDown(int key) const;
    bool isKeyPressed(int key) const;  // This frame only
    bool isKeyReleased(int key) const;
    
    // Mouse
    bool isMouseButtonDown(MouseButton button) const;
    glm::vec2 getMousePosition() const;
    glm::vec2 getMouseDelta() const;
    float getMouseScrollDelta() const;
    
    // Gamepad (future)
    bool isGamepadConnected(int gamepadId) const;
    float getGamepadAxis(int gamepadId, int axis) const;
    
    // Frame update
    void update();
    
private:
    GLFWwindow* m_window;
    std::unordered_set<int> m_keysDown;
    std::unordered_set<int> m_keysPressedThisFrame;
    std::unordered_set<int> m_keysReleasedThisFrame;
    
    glm::vec2 m_mousePos{0.0f};
    glm::vec2 m_mouseDelta{0.0f};
    float m_scrollDelta{0.0f};
};

} // namespace Artifact::Input
```

**Lua Bindings (Wrapper):**

```cpp
// EngineCore/Scripting/LuaBindings.cpp
void LuaBindings::registerInputBindings() {
    auto& lua = m_context.state();
    sol::table input = ensureInputTable();
    
    input.set_function("IsKeyDown", [this](int key) {
        return m_inputSystem->isKeyDown(key);
    });
    
    input.set_function("GetMousePosition", [this]() {
        auto pos = m_inputSystem->getMousePosition();
        return lua.create_table_with("x", pos.x, "y", pos.y);
    });
}
```

---

### 5. Component System Improvements

#### 5.1 Component Registry Pattern

```cpp
// EngineCore/include/Artifact/Scene/ComponentRegistry.h
namespace Artifact::Scene {

class ComponentRegistry {
public:
    template<typename T>
    static void registerComponent(const std::string& name) {
        s_factories[name] = []() -> ComponentPtr {
            return std::make_unique<T>();
        };
    }
    
    static ComponentPtr create(const std::string& name);
    static std::vector<std::string> getAllComponentNames();
    
private:
    using ComponentPtr = std::unique_ptr<void>; // Type-erased
    using Factory = std::function<ComponentPtr()>;
    static std::unordered_map<std::string, Factory> s_factories;
};

} // namespace Artifact::Scene
```

**Usage:**

```cpp
// In Application::Application()
Scene::ComponentRegistry::registerComponent<Scene::CameraComponent>("Camera");
Scene::ComponentRegistry::registerComponent<Scene::MeshRendererComponent>("MeshRenderer");
Scene::ComponentRegistry::registerComponent<Physics::RigidBody2DComponent>("RigidBody2D");
```

---

### 6. Asset Management Overhaul

#### 6.1 Asset Handle System

```cpp
// EngineCore/include/Artifact/Assets/AssetHandle.h
namespace Artifact::Assets {

using AssetID = uint64_t;

template<typename T>
class AssetHandle {
public:
    AssetHandle() = default;
    explicit AssetHandle(AssetID id) : m_id(id) {}
    
    T* get() const;
    bool isValid() const;
    AssetID id() const { return m_id; }
    
private:
    AssetID m_id{0};
};

class AssetManager {
public:
    template<typename T>
    AssetHandle<T> load(const std::filesystem::path& path);
    
    template<typename T>
    void unload(AssetHandle<T> handle);
    
    template<typename T>
    T* get(AssetHandle<T> handle);
    
private:
    struct AssetEntry {
        std::unique_ptr<void> data;
        std::string path;
        size_t refCount{0};
    };
    
    std::unordered_map<AssetID, AssetEntry> m_assets;
    AssetID m_nextId{1};
};

} // namespace Artifact::Assets
```

#### 6.2 Asset Types

```cpp
// EngineCore/include/Artifact/Assets/TextureAsset.h
namespace Artifact::Assets {

class TextureAsset {
public:
    TextureAsset(const std::filesystem::path& path);
    ~TextureAsset();
    
    GLuint handle() const { return m_textureId; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    
private:
    GLuint m_textureId{0};
    int m_width{0}, m_height{0};
};

} // namespace Artifact::Assets
```

---

### 7. Lua/C++ Boundary Definition

#### Allowed in Lua:
✅ **Game Logic**
- AI behaviors (state machines, decision trees)
- Quest/dialogue systems
- UI event handlers
- Gameplay rules (scoring, win conditions)
- Prototype behaviors

✅ **High-Level APIs**
- Entity.Create("Prefab")
- Entity.GetComponent("Transform")
- Physics.ApplyForce(...)
- Audio.Play("sound.wav")

#### Forbidden in Lua:
❌ **Performance-Critical Systems**
- Rendering loops
- Physics simulation
- Pathfinding (A* should be C++)
- Collision detection (broad phase)

❌ **Engine Features**
- Camera implementation
- Asset loading
- Memory management
- Low-level input polling

**Enforcement:**
- Document clearly in `LUA_API_REFERENCE.md`
- Provide C++ alternatives for common patterns
- Editor warnings for slow Lua scripts (Tracy integration)

---

## Migration Strategy

### Phase 1: Camera System (Week 1)
1. Implement `Camera` class in C++
2. Add `CameraComponent`
3. Create `CameraSystem`
4. Add Lua bindings for prototyping
5. Update `ScenePanel` to use C++ camera
6. **Deprecate** `CameraController.lua` (keep as example)

### Phase 2: Rendering Pipeline (Weeks 2-3)
1. Implement `Mesh`, `Material`, `Shader` classes
2. Create `MeshRendererComponent`
3. Build `RenderQueue` and sorting
4. Integrate with `Renderer::renderScene()`
5. Test with basic cube/sphere primitives

### Phase 3: Physics Integration (Week 4)
1. Add physics components
2. Wire up Box2D in `PhysicsSystem::update2D()`
3. Expose Lua API for physics
4. Create physics debugging visualizer

### Phase 4: Input System (Week 5)
1. Implement `InputSystem`
2. Migrate from global Lua functions
3. Add gamepad support

### Phase 5: Asset Pipeline (Weeks 6-8)
1. Texture compression (BC7 via DirectXTex or compressonator)
2. Model import (Assimp → custom format)
3. Shader compilation (glslang)

### Phase 6: Audio System (Weeks 9-10)
1. Integrate miniaudio or OpenAL Soft
2. Add `AudioSource` component
3. Lua API for sound playback

---

## Breaking Changes

### For Existing Scripts:
- `CameraController.lua` → Migrate to C++ or use new Lua API
- `ExampleBehaviour.lua` → Update to new component API

### For Editor:
- Inspector panel needs component reflection system
- Scene serialization format will change

---

## Long-Term Vision

### Version 0.3.0 (6 months)
- **PBR Rendering**: Image-based lighting, shadow maps
- **Animation System**: Skeletal animation, blend trees
- **Particle System**: GPU-based particles
- **Networking**: Replication layer for multiplayer

### Version 1.0.0 (12 months)
- **Visual Scripting**: Blueprint-like system
- **Advanced Editor**: Prefab variants, undo/redo
- **Build Pipeline**: One-click packaging for Steam/itch.io
- **Documentation**: Full API reference, video tutorials

---

## Success Metrics

- ✅ Camera updates run at <0.1ms (Tracy measurement)
- ✅ 10,000+ entities at 60 FPS
- ✅ Lua scripts can be hot-reloaded without crashes
- ✅ Build time <2 minutes (incremental)
- ✅ Zero warnings on `-Wall -Wextra -Wpedantic`

---

## Questions for Consideration

1. **Scripting Language**: Keep LuaJIT or add C# (Mono)?
2. **Rendering API**: OpenGL only or add Vulkan/DX12?
3. **ECS Alternative**: Stay with EnTT or try Flecs?
4. **Editor Framework**: ImGui or switch to Qt/Dear PyGui?

---

## Next Steps

1. Review this proposal with team
2. Create GitHub issues for Phase 1-3 tasks
3. Set up project board (Kanban)
4. Write unit tests for new systems before implementation
5. Update `CMakeLists.txt` for new modules

**Approval Required Before Proceeding** ✋


