# Lua/C++ Boundary Guidelines

**Purpose:** Define clear rules for what belongs in C++ vs Lua to maintain performance, maintainability, and rapid iteration.

---

## Philosophy

> **"C++ is the foundation, Lua is the paintbrush."**

- **C++** provides the **performance-critical engine systems** and **stable APIs**
- **Lua** provides **game logic flexibility** and **rapid prototyping**
- The boundary should be **explicit**, **well-documented**, and **minimal in overhead**

---

## Decision Framework

### Question: "Should this be in C++ or Lua?"

```
START
  |
  ├─> Is it a hot path (runs every frame for many entities)?
  |   └─> YES → C++ (with optional Lua wrapper)
  |
  ├─> Does it require direct hardware access (GPU, physics, audio)?
  |   └─> YES → C++
  |
  ├─> Is it game-specific logic (quest, AI, dialogue)?
  |   └─> YES → Lua
  |
  ├─> Does it benefit from hot-reload during development?
  |   └─> YES → Lua (prototype first, move to C++ if needed)
  |
  └─> Default → Start in Lua, profile, move to C++ if bottleneck
```

---

## C++ Responsibilities

### 1. Core Engine Systems ✅

**Rendering Pipeline**
- Camera management (view/projection matrices)
- Render queue sorting
- Mesh/Material/Shader management
- Framebuffer operations
- GPU resource upload

**Physics Simulation**
- Broad/narrow phase collision detection
- Rigid body integration
- Constraint solving
- Raycasting/shape queries

**Asset Management**
- File loading (textures, models, audio)
- Asset cooking (compression, format conversion)
- Memory management (pools, allocators)
- Hot-reload infrastructure

**Input Handling**
- Raw input polling (keyboard, mouse, gamepad)
- Input mapping (actions, axes)
- Event queue processing

**Audio System**
- Sound source management
- 3D audio spatialization
- DSP effects
- Streaming/decoding

### 2. Performance-Critical Gameplay ✅

**Pathfinding**
- A* / Dijkstra implementations
- Navigation mesh generation
- Crowd simulation (if >100 agents)

**Particle Systems**
- GPU-based particle simulation
- Emitter management

**Animation**
- Skeletal animation blending
- IK solvers
- Animation compression

### 3. Platform-Specific Code ✅

- File I/O (async, memory-mapped)
- Networking sockets
- Threading primitives
- Profiling hooks

---

## Lua Responsibilities

### 1. Game Logic ✅

**AI Behaviors**
```lua
-- Finite state machines
local Enemy = {}

function Enemy:update(dt)
    if self.state == "patrol" then
        self:patrol(dt)
    elseif self.state == "chase" then
        self:chasePlayer(dt)
    elseif self.state == "attack" then
        self:attack(dt)
    end
end

return Enemy
```

**Quest Systems**
```lua
local QuestManager = {
    quests = {}
}

function QuestManager:startQuest(questId)
    local quest = self.quests[questId]
    quest.status = "active"
    quest.progress = 0
end

function QuestManager:checkConditions()
    for _, quest in pairs(self.quests) do
        if quest.status == "active" and quest:isComplete() then
            quest:onComplete()
        end
    end
end
```

**Dialogue Trees**
```lua
local DialogueTree = {
    nodes = {
        start = {
            text = "Hello, traveler!",
            choices = {
                {text = "Who are you?", next = "intro"},
                {text = "Goodbye", next = "end"}
            }
        }
    }
}
```

### 2. UI Logic ✅

**HUD Updates**
```lua
function HUD:update()
    self.healthBar:setValue(Player.GetHealth())
    self.ammoText:setText(string.format("%d / %d", Player.GetAmmo(), Player.GetMaxAmmo()))
end
```

**Menu Navigation**
```lua
function MainMenu:onPlayButton()
    Scene.Load("Levels/Level1.scene")
end
```

### 3. Gameplay Tuning ✅

**Character Stats**
```lua
local PlayerStats = {
    maxHealth = 100,
    moveSpeed = 5.0,
    jumpForce = 10.0,
    attackDamage = 25
}
```

**Weapon Definitions**
```lua
local Weapons = {
    pistol = {
        damage = 15,
        fireRate = 0.5,
        clipSize = 12,
        reloadTime = 1.2
    },
    shotgun = {
        damage = 50,
        fireRate = 1.0,
        clipSize = 6,
        reloadTime = 2.0
    }
}
```

### 4. Rapid Prototyping ✅

**Experimental Mechanics**
```lua
-- Prototype a grappling hook in Lua first
function GrapplingHook:fire(direction)
    local hit = Physics.Raycast(self.position, direction, 50.0)
    if hit then
        self.ropeLength = glm.distance(self.position, hit.point)
        self.mode = "swinging"
    end
end

-- If it becomes a core mechanic, rewrite in C++
```

---

## The Boundary Layer: Lua Bindings

### Best Practices

#### ✅ DO: Expose High-Level APIs
```cpp
// Good: Semantic, easy to use from Lua
lua["Entity"]["Translate"] = [](float x, float y, float z) {
    // Internal C++ logic
};
```

#### ❌ DON'T: Expose Raw Pointers
```cpp
// Bad: Unsafe, error-prone
lua["Entity"]["GetTransform"] = []() {
    return reinterpret_cast<uintptr_t>(GetActiveTransform());
};
```

#### ✅ DO: Validate Input
```cpp
lua["Physics"]["ApplyForce"] = [](float x, float y, float z) {
    if (!IsFinite(x) || !IsFinite(y) || !IsFinite(z)) {
        spdlog::warn("Invalid force vector: ({}, {}, {})", x, y, z);
        return;
    }
    // Apply force
};
```

#### ✅ DO: Provide Batch APIs
```cpp
// Instead of calling Lua → C++ 100 times:
lua["Renderer"]["DrawInstances"] = [](sol::table transforms, MeshHandle mesh) {
    std::vector<glm::mat4> matrices;
    for (auto& [key, value] : transforms) {
        matrices.push_back(value.as<glm::mat4>());
    }
    DrawInstanced(mesh, matrices);
};
```

#### ❌ DON'T: Call Lua from Hot Paths
```cpp
// Bad: Lua callback in tight loop
for (Entity entity : entities) {
    luaFunction(entity); // <-- SLOW
}

// Good: Let Lua iterate
lua["ProcessEntities"] = [](sol::table entityIds) {
    for (auto& [key, id] : entityIds) {
        ProcessEntity(id.as<int>());
    }
};
```

---

## Performance Implications

### Lua → C++ Call Overhead

**Typical Costs:**
- Function call: ~50-100ns (sol2)
- Type conversion: ~10-20ns per argument
- Error checking: ~30ns

**Example:**
```cpp
// Measured with Tracy
lua["Entity"]["SetPosition"](10.0f, 20.0f, 30.0f);
// Total: ~150ns
```

**Impact:**
- 10,000 entities calling Lua → C++ every frame = **1.5ms**
- Solution: **Batch operations** or **move to C++ systems**

### C++ → Lua Call Overhead

**Typical Costs:**
- Function call: ~100-200ns
- Garbage collection (if triggered): ~1-10ms

**Rule:** Avoid calling Lua from C++ in hot paths.

---

## Migration Path: Lua → C++

### When to Migrate:
1. **Profiler shows Lua script taking >1ms per frame**
2. **Script runs on 1000+ entities**
3. **Feature is now core to the engine (e.g., camera)**

### Steps:
1. **Write unit tests** for Lua behavior
2. **Implement equivalent C++ component/system**
3. **Add Lua bindings** to new C++ API
4. **Deprecate old Lua script** (move to `Deprecated/` folder)
5. **Update documentation**

### Example: Camera Migration

**Before (Lua):**
```lua
-- CameraController.lua
function CameraController:update(dt)
    local moveDir = Input.GetAxis("Horizontal")
    Entity.Translate(moveDir * self.speed * dt, 0, 0)
end
```

**After (C++ Component):**
```cpp
// CameraComponent with C++ logic
struct CameraComponent {
    Camera camera;
    float moveSpeed{5.0f};
};

// System updates all cameras in C++
void CameraSystem::update(entt::registry& reg, float dt) {
    auto view = reg.view<CameraComponent, TransformComponent>();
    for (auto [entity, cam, transform] : view.each()) {
        float moveDir = Input::GetAxis("Horizontal");
        transform.position.x += moveDir * cam.moveSpeed * dt;
    }
}
```

**Lua API (Optional):**
```lua
-- Still allow Lua to control camera for prototyping
Camera.SetSpeed(10.0)
Camera.SetPosition(0, 10, 20)
```

---

## Component Design: C++ vs Lua

### Hybrid Approach (Recommended)

```cpp
// C++ Component (data-oriented)
struct CharacterComponent {
    float health{100.0f};
    float maxHealth{100.0f};
    float moveSpeed{5.0f};
    
    // Internal engine state
    glm::vec3 velocity{0.0f};
    bool isGrounded{false};
};

// Lua Script (behavior)
-- CharacterBehavior.lua
function CharacterBehavior:update(dt)
    local character = Entity.GetComponent("Character")
    
    -- Game-specific logic in Lua
    if Input.IsKeyPressed("Space") and character.isGrounded then
        Physics.ApplyImpulse(0, character.jumpForce, 0)
    end
    
    -- Health regeneration (gameplay logic)
    if character.health < character.maxHealth then
        character.health = character.health + self.regenRate * dt
    end
end
```

**Advantages:**
- **Performance:** C++ handles physics, Lua handles high-level logic
- **Flexibility:** Tweak `regenRate` in Lua without recompile
- **Clarity:** Data in C++, behavior in Lua

---

## API Design Guidelines

### Naming Conventions

**C++ (Engine API):**
```cpp
namespace Artifact::Rendering {
    class Camera {
        void setPosition(const glm::vec3& pos); // camelCase methods
    };
}
```

**Lua (Game API):**
```lua
-- PascalCase for namespaces, actions
Camera.SetPosition(0, 10, 20)
Entity.AddComponent("RigidBody")
Physics.ApplyForce({x=10, y=0, z=0})
```

### Type Safety

**Use sol2's type checking:**
```cpp
lua.set_function("SetHealth", [](float health) {
    if (health < 0.0f) {
        throw std::invalid_argument("Health cannot be negative");
    }
    // ...
});
```

**In Lua:**
```lua
-- This will throw C++ exception, caught by Lua
local success, err = pcall(function()
    Entity.SetHealth(-10) -- ERROR
end)
if not success then
    print("Error: " .. err)
end
```

---

## Documentation Requirements

### For Every Lua API:
1. **Function signature** with types
2. **Description** of behavior
3. **Example usage**
4. **Performance notes** (if any)

**Template:**
```markdown
### Entity.SetPosition(x: number, y: number, z: number)

Sets the world position of the current entity.

**Parameters:**
- `x`, `y`, `z`: World coordinates (float)

**Example:**
```lua
function MyScript:start()
    Entity.SetPosition(0, 10, 0) -- Place 10 units above ground
end
```

**Performance:** O(1), ~100ns overhead
```

---

## Case Studies

### Case Study 1: Camera System ✅ Correct

**Decision:** Move from Lua to C++

**Rationale:**
- Runs every frame for every camera
- Requires matrix math (better in C++)
- Core engine feature, not game-specific

**Result:**
- Performance: 10ms → 0.01ms (1000x improvement)
- Maintainability: Easier to debug in C++
- Flexibility: Lua can still override for prototypes

---

### Case Study 2: Enemy AI ✅ Correct

**Decision:** Keep in Lua

**Rationale:**
- Game-specific logic (not engine feature)
- Needs rapid iteration (designers tweak)
- Not performance-critical (<100 enemies)

**Result:**
- Iteration speed: 0s (no recompile)
- Flexibility: Easy for non-programmers to modify

---

### Case Study 3: Particle Systems ❌ Incorrect (Initial)

**Initial:** Implemented in Lua

**Problem:** 10,000 particles → 50ms per frame

**Solution:** Rewrite in C++ with GPU compute shader

**Lesson:** Prototype in Lua, migrate to C++ when scaling

---

## Checklist for New Features

Before adding a feature, ask:

- [ ] Does this run every frame?
- [ ] Does it affect >100 entities?
- [ ] Is it game-specific or engine-generic?
- [ ] Do designers need to tweak it frequently?
- [ ] Does it require hardware access?
- [ ] Is there a performance target? (e.g., <1ms)

**If 3+ checks → C++, otherwise → Lua**

---

## Future: Visual Scripting?

**Option:** Add Blueprint-like visual scripting (e.g., ImNodes)

**Pros:**
- Non-programmer friendly
- Visual dataflow easier to debug

**Cons:**
- More complex to implement
- Another layer to maintain

**Recommendation:** Revisit after Lua/C++ boundary is stable (v0.3.0+)

---

## Summary Table

| Feature | C++ | Lua | Why |
|---------|-----|-----|-----|
| **Camera** | ✅ | ❌ | Performance, core system |
| **Physics Sim** | ✅ | ❌ | Hardware-dependent |
| **Rendering** | ✅ | ❌ | GPU access required |
| **Enemy AI** | ❌ | ✅ | Game-specific, iteration |
| **Quest Logic** | ❌ | ✅ | Designer-owned content |
| **Input Polling** | ✅ | Wrapper | Low-level, but Lua needs access |
| **Audio Playback** | ✅ | Wrapper | Hardware, but Lua triggers |
| **HUD Updates** | ❌ | ✅ | UI logic, frequent changes |
| **Animation Blending** | ✅ | ❌ | Math-heavy, performance |
| **Dialogue Trees** | ❌ | ✅ | Content, not code |

---

## Conclusion

**The best architecture uses both C++ and Lua to their strengths:**
- C++ provides the **engine** (fast, safe, stable)
- Lua provides the **game** (flexible, fast iteration, accessible)

By maintaining a clear boundary, you achieve:
- ⚡ **Performance** where it matters
- 🚀 **Iteration speed** for gameplay
- 🛡️ **Type safety** in critical paths
- 🎨 **Creativity** for designers and scripters


