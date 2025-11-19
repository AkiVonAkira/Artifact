# Package Manager Comparison: vcpkg vs Alternatives

**Current:** vcpkg  
**Status:** Working, but has pain points  
**Question:** Should we switch?

---

## TL;DR Recommendation

**🎯 Keep vcpkg, but optimize:**
1. Enable binary caching
2. Use Docker for CI/CD
3. Pin versions in manifest
4. Selective feature flags

**Why not switch?**
- Migration cost is HIGH (3-5 days)
- vcpkg works well for your use case
- Alternatives have their own trade-offs
- Time better spent on engine features

---

## Current Setup Analysis

### What You're Using (vcpkg.json)

```json
{
  "dependencies": [
    "assimp",        // ~5 min compile
    "box2d",         // ~30s
    "catch2",        // ~1 min
    "efsw",          // ~30s
    "entt",          // Header-only, fast
    "glad",          // Generated, fast
    "glfw3",         // ~1 min
    "glm",           // Header-only, fast
    "imgui",         // ~2 min (with features)
    "imguizmo",      // ~30s
    "joltphysics",   // ~10 min (large)
    "luajit",        // ~2 min
    "sol2",          // Header-only, fast
    "spdlog",        // Header-only, fast
    "stb",           // Header-only
    "tracy"          // ~2 min
  ]
}
```

**Total Build Time (Cold):** ~25-30 minutes  
**Disk Usage:** ~2-3 GB (per triplet)

---

## vcpkg: Deep Dive

### ✅ Advantages

1. **Official Microsoft Support**
   - Well-maintained, active development
   - Corporate backing (not going away)

2. **Seamless CMake Integration**
   ```cmake
   find_package(glfw3 CONFIG REQUIRED)
   target_link_libraries(MyTarget PRIVATE glfw)
   ```
   - No manual path wrangling
   - Works out of the box

3. **Version Locking (Manifests)**
   - `vcpkg.json` ensures reproducible builds
   - Built-in baseline support

4. **Cross-Platform**
   - Windows, Linux, macOS supported
   - Handles platform quirks (e.g., pkgconf for LuaJIT)

5. **Large Ecosystem**
   - 2000+ packages
   - All your dependencies are available

### ❌ Disadvantages

1. **Slow Builds from Source**
   - Every package compiles locally
   - No binary cache by default
   - 30+ min initial setup

2. **Disk Space Hog**
   - Each triplet (debug/release) duplicates libraries
   - Can reach 10+ GB for complex projects

3. **Dependency Hell**
   - Transitive dependencies can conflict
   - Hard to override specific versions

4. **Limited Versioning**
   - Can't specify exact Git commits easily
   - Baseline = snapshot of vcpkg repo, not per-package

5. **Opaque Errors**
   - Build failures are hard to debug
   - Cryptic CMake errors

---

## Alternative 1: Conan 2.0

### Overview
- **Type:** Binary package manager
- **Language:** Python (recipes)
- **Ecosystem:** 1500+ packages (growing)

### ✅ Pros

1. **Binary Packages by Default**
   - Download pre-compiled binaries from ConanCenter
   - 10-100x faster than vcpkg (2-3 min vs 30 min)

2. **Flexible Versioning**
   ```python
   # conanfile.txt
   [requires]
   glfw/3.3.8
   imgui/1.89.9
   assimp/5.2.5@user/stable
   ```
   - Semantic versioning
   - Can specify Git revisions

3. **Profile-Based Configuration**
   - Per-project settings (compiler, flags, etc.)
   - Easy to switch between configurations

4. **Better for Proprietary Deps**
   - Can host private packages on Artifactory/Nexus
   - Built-in support for internal repos

5. **Lockfiles**
   - `conan.lock` ensures exact reproducibility
   - Better than vcpkg baselines

### ❌ Cons

1. **Learning Curve**
   - Requires understanding of profiles, remotes, recipes
   - More complex than vcpkg

2. **Smaller Ecosystem**
   - Not all packages have pre-built binaries
   - May need to build locally anyway

3. **CMake Integration is Manual**
   ```cmake
   include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
   conan_basic_setup()
   ```
   - Not as clean as vcpkg

4. **Breaking Changes (1.x → 2.x)**
   - Conan 2.0 is new (2023)
   - Some packages not updated yet

### Migration Effort: **Medium (2-3 days)**

#### Steps:
1. Install Conan: `pip install conan`
2. Create `conanfile.txt`:
   ```
   [requires]
   glfw/3.3.8
   imgui/1.89@bincrafters/stable
   
   [generators]
   CMakeDeps
   CMakeToolchain
   ```
3. Update `CMakeLists.txt`:
   ```cmake
   list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
   find_package(glfw3 REQUIRED)
   ```
4. Run `conan install . --build=missing`

---

## Alternative 2: CPM (CMake Package Manager)

### Overview
- **Type:** Header-only CMake wrapper around FetchContent
- **Language:** Pure CMake
- **Ecosystem:** Any CMake-based project

### ✅ Pros

1. **Zero External Dependencies**
   - Just include `CPM.cmake` in your repo
   - No separate tool to install

2. **Git-Based Versioning**
   ```cmake
   CPMAddPackage(
     NAME glfw
     GITHUB_REPOSITORY glfw/glfw
     GIT_TAG 3.3.8
   )
   ```
   - Pin exact commits
   - Easy to use forks

3. **Transparent**
   - Everything is visible in CMake
   - No hidden magic

4. **Fast for Header-Only Libs**
   - EnTT, GLM, sol2, spdlog → instant

### ❌ Cons

1. **No Pre-Built Binaries**
   - Always builds from source
   - Slow for large libraries (Assimp, Jolt)

2. **Pollutes CMake**
   - Every dependency's CMake targets are visible
   - Can cause conflicts

3. **Not Suitable for Large Projects**
   - 10+ dependencies = CMake soup

### Best Use Case: **Supplement vcpkg**

```cmake
# Use vcpkg for big libraries
find_package(assimp CONFIG REQUIRED)

# Use CPM for small, header-only libs
CPMAddPackage(
  NAME magic_enum
  GITHUB_REPOSITORY Neargye/magic_enum
  VERSION 0.9.3
)
```

---

## Alternative 3: System Package Managers

### apt (Debian/Ubuntu)

```bash
sudo apt install libglfw3-dev libglm-dev libassimp-dev
```

### Homebrew (macOS)

```bash
brew install glfw glm assimp
```

### ❌ Why Not?

1. **Version Control:** System packages are often outdated
2. **Portability:** Breaks cross-platform builds
3. **Isolation:** System-wide installations cause conflicts
4. **CI/CD:** Can't pin versions reliably

**Verdict:** Not viable for serious projects

---

## Alternative 4: Git Submodules

### Overview
- Manually vendor dependencies as submodules

### ✅ Pros
- Full control over versions
- No external tool dependency

### ❌ Cons
- Manual work (configure, build each dep)
- No dependency resolution
- Hard to update

**Verdict:** Only for tiny projects (<5 deps)

---

## Hybrid Approach: Best of Both Worlds

### Strategy: vcpkg + CPM

```cmake
# Heavy libraries via vcpkg
find_package(assimp CONFIG REQUIRED)
find_package(joltphysics CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Light/header-only via CPM
include(cmake/CPM.cmake)

CPMAddPackage("gh:fmtlib/fmt#9.1.0")
CPMAddPackage("gh:gabime/spdlog#v1.11.0")
CPMAddPackage("gh:Neargye/magic_enum#v0.9.3")
```

### Advantages:
- Fast setup (vcpkg binary cache + CPM for small libs)
- Flexibility (can fork CPM deps easily)
- Best tool for each job

---

## Optimization: vcpkg Binary Caching

### Problem: Cold Builds Take 30+ Minutes

### Solution: Enable Binary Caching

#### Step 1: Create Cache Directory
```bash
mkdir -p ~/.vcpkg/cache
```

#### Step 2: Configure vcpkg
```bash
# Windows (PowerShell)
$env:VCPKG_BINARY_SOURCES="files,$HOME\.vcpkg\cache,readwrite"

# Linux/macOS
export VCPKG_BINARY_SOURCES="files,$HOME/.vcpkg/cache,readwrite"
```

#### Step 3: Add to CMakePresets.json
```json
{
  "configurePresets": [
    {
      "name": "base",
      "environment": {
        "VCPKG_BINARY_SOURCES": "files,$env{HOME}/.vcpkg/cache,readwrite"
      }
    }
  ]
}
```

### Result:
- First build: 30 minutes (cache is empty)
- Subsequent builds: **2-3 minutes** (cache hit)
- Shared across projects on same machine

---

## Optimization: Docker CI/CD

### Problem: CI Rebuilds Dependencies Every Time

### Solution: Multi-Stage Docker Build

#### Dockerfile
```dockerfile
# Stage 1: Build dependencies
FROM ubuntu:22.04 AS deps
RUN apt-get update && apt-get install -y cmake ninja-build git curl zip unzip tar

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /vcpkg
RUN /vcpkg/bootstrap-vcpkg.sh

# Copy manifest and install deps
COPY vcpkg.json /workspace/
WORKDIR /workspace
RUN /vcpkg/vcpkg install --triplet=x64-linux

# Stage 2: Build project
FROM deps AS build
COPY . /workspace
RUN cmake --preset linux-release
RUN cmake --build build/linux-release

# Stage 3: Runtime
FROM ubuntu:22.04
COPY --from=build /workspace/build/linux-release/Editor/ArtifactEditor /app/
CMD ["/app/ArtifactEditor"]
```

### Benefits:
- Dependencies cached in `deps` layer
- Rebuilds only take 1-2 min
- Consistent CI environment

---

## Optimization: Selective Features

### Problem: ImGui Builds Too Many Features

### Solution: Minimal Features

#### Current (vcpkg.json)
```json
{
  "name": "imgui",
  "features": [
    "docking-experimental",
    "glfw-binding",
    "opengl3-binding"
  ]
}
```

**Build Time:** ~2 minutes

#### Optimized
```json
{
  "name": "imgui",
  "default-features": false,
  "features": [
    "docking-experimental",
    "glfw-binding",
    "opengl3-binding"
  ]
}
```

**Build Time:** ~1 minute (50% faster)

---

## Recommendation Matrix

| Criteria | vcpkg | Conan | CPM | Submodules |
|----------|-------|-------|-----|------------|
| **Setup Time** | 🟡 30min | 🟢 3min | 🟢 1min | 🔴 Hours |
| **CMake Integration** | 🟢 Perfect | 🟡 Good | 🟢 Native | 🟡 Manual |
| **Versioning** | 🟡 Baseline | 🟢 SemVer | 🟢 Git tags | 🟢 Manual |
| **Binary Cache** | 🟡 Manual | 🟢 Built-in | 🔴 None | 🔴 N/A |
| **Ecosystem** | 🟢 2000+ | 🟡 1500+ | 🟢 Any CMake | 🟡 Manual |
| **Learning Curve** | 🟢 Easy | 🟡 Medium | 🟢 Easy | 🟡 Medium |
| **Cross-Platform** | 🟢 Excellent | 🟢 Excellent | 🟢 Good | 🟡 Manual |

---

## Action Plan

### Immediate (This Week)
1. ✅ Enable vcpkg binary caching (5 min)
2. ✅ Add CPM for header-only libs (15 min)
3. ✅ Document build times in README

### Short-Term (This Month)
4. ✅ Create Docker CI image (2 hours)
5. ✅ Pin vcpkg baseline in manifest (10 min)
6. ✅ Profile build with Tracy to find bottlenecks

### Long-Term (If Needed)
7. ⏸️ Evaluate Conan migration (only if vcpkg becomes a blocker)

---

## Conclusion

**Stick with vcpkg for now.** 

**Why?**
- It works
- Migration cost > benefit
- Optimizations (binary cache, Docker) solve 80% of pain points
- Focus on engine features, not build system

**When to Reconsider?**
- vcpkg becomes unmaintained (unlikely)
- A dependency is not available in vcpkg
- Build times exceed 10 min even with cache
- Team grows and needs better binary distribution

---

## Further Reading

- [vcpkg Binary Caching](https://learn.microsoft.com/en-us/vcpkg/users/binarycaching)
- [Conan 2.0 Tutorial](https://docs.conan.io/2/)
- [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)


