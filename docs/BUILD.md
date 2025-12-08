# Building Artifact Engine

This document explains how to build Artifact Engine on Windows, Linux, and macOS.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [CMake Presets](#cmake-presets)
- [Build Options](#build-options)
- [vcpkg Integration](#vcpkg-integration)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### All Platforms

- **CMake** 3.24 or newer
- **C++23 compiler**:
  - Windows: Visual Studio 2022 (MSVC 19.30+)
  - Linux: GCC 11+ or Clang 13+
  - macOS: Xcode 14+ (AppleClang 14+)
- **vcpkg** (recommended) or system package manager

### Installing vcpkg

```bash
# Clone vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
# Windows:
.\bootstrap-vcpkg.bat
# Linux/macOS:
./bootstrap-vcpkg.sh

# Set environment variable
# Windows (PowerShell):
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
[System.Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'C:\path\to\vcpkg', 'User')

# Linux/macOS (add to ~/.bashrc or ~/.zshrc):
export VCPKG_ROOT=/path/to/vcpkg
```

---

## Quick Start

### Windows

```powershell
# Configure
cmake --preset debug

# Build
cmake --build --preset debug-build

# Run
.\build\debug\bin\Windows\Debug\ArtifactEditor.exe
```

### Linux / macOS

```bash
# Configure
cmake --preset debug

# Build
cmake --build --preset debug-build

# Run
./build/debug/bin/Linux/Debug/ArtifactEditor  # or macOS
```

---

## CMake Presets

Artifact uses minimal, cross-platform CMake presets. No platform-specific duplicates!

### Available Presets

| Preset | Description | Tracy | Tests |
|--------|-------------|-------|-------|
| `debug` | Debug build with all features | ✓ | ✗ |
| `release` | Optimized release build | ✗ | ✗ |
| `relwithdebinfo` | Release with debug symbols | ✗ | ✗ |
| `tests-debug` | Debug build for running tests | ✗ | ✓ |

### Usage

```bash
# Configure
cmake --preset <preset-name>

# Build
cmake --build --preset <preset-name>-build

# Run tests (tests-debug only)
ctest --preset tests-debug-ctest
```

### How Presets Work

- **Generator**: Automatically chosen by CMake or your IDE (Ninja, Make, VS, Xcode)
- **vcpkg**: Automatically used if `VCPKG_ROOT` is set
- **Platform**: Detected automatically (Windows/Linux/macOS)

---

## Build Options

You can override options via `-D` flags:

```bash
cmake --preset debug \
  -DARTIFACT_ENABLE_EDITOR=ON \
  -DARTIFACT_ENABLE_RUNTIME=OFF \
  -DARTIFACT_ENABLE_TESTS=ON \
  -DARTIFACT_ENABLE_TRACY=ON \
  -DARTIFACT_ENABLE_TOOLS=OFF \
  -DARTIFACT_BUILD_PACKAGING=OFF
```

### Component Options

- `ARTIFACT_ENABLE_EDITOR` (default: `ON`) - Build the editor executable
- `ARTIFACT_ENABLE_RUNTIME` (default: `OFF`) - Build standalone runtime
- `ARTIFACT_ENABLE_TESTS` (default: `ON` for tests-debug, `OFF` otherwise)
- `ARTIFACT_ENABLE_TOOLS` (default: `OFF`) - Build developer tools
- `ARTIFACT_BUILD_PACKAGING` (default: `OFF`) - Enable packaging/installer

### Feature Options

- `ARTIFACT_ENABLE_TRACY` (default: `ON` for debug, `OFF` for release) - Tracy profiler support
- `ARTIFACT_USE_UNITY_BUILD` (default: `OFF`) - Unity/jumbo builds for faster compilation

---

## vcpkg Integration

### Why vcpkg?

vcpkg is the **best** option for C++ dependency management:

✅ **Cross-platform**: Works on Windows, Linux, macOS  
✅ **Integrated with CMake**: Zero configuration needed  
✅ **Caching**: Built packages are cached, no rebuild unless manifest changes  
✅ **Reproducible**: Lock file ensures consistent builds  
✅ **No manual downloads**: Everything is automated  

### How It Works

1. **`vcpkg.json`** - Lists all dependencies
2. **CMake detects vcpkg** - Via `VCPKG_ROOT` environment variable
3. **vcpkg installs packages** - On first configure (cached for subsequent runs)
4. **CMake finds packages** - Using `find_package()`

### vcpkg Caching

vcpkg **caches built packages**. You only rebuild when:

- Changing `vcpkg.json` (adding/removing dependencies)
- Deleting `build/` directory
- Updating vcpkg itself

**To avoid rebuilding everything:**

```bash
# DON'T delete entire build/
rm -rf build/

# DO delete CMake cache only:
rm build/debug/CMakeCache.txt
cmake --preset debug
```

### Updating Dependencies

```bash
# Update vcpkg itself
cd $VCPKG_ROOT
git pull
./bootstrap-vcpkg.sh  # or .bat on Windows

# Update packages
cd /path/to/Artifact
vcpkg upgrade --no-dry-run
```

---

## Troubleshooting

### "No generator specified"

CMake couldn't detect a generator. Specify one:

```bash
cmake --preset debug -G Ninja
# or
cmake --preset debug -G "Visual Studio 17 2022"
```

### "vcpkg install failed"

Check `build/<preset>/vcpkg-manifest-install.log`.

Common fixes:

```bash
# Clean vcpkg build cache for a specific package
rm -rf build/debug/vcpkg_installed/vcpkg/blds/<package-name>

# Re-run configure
cmake --preset debug
```

### Compiler not found (Linux)

```bash
# Install compilers
# Debian/Ubuntu:
sudo apt install build-essential cmake ninja-build

# Fedora:
sudo dnf install gcc-c++ cmake ninja-build

# Arch:
sudo pacman -S base-devel cmake ninja
```

### Missing dependencies (Linux)

Some system libraries are needed:

```bash
# Debian/Ubuntu:
sudo apt install libx11-dev libxrandr-dev libxinerama-dev \
                 libxcursor-dev libxi-dev libgl1-mesa-dev

# Fedora:
sudo dnf install libX11-devel libXrandr-devel libXinerama-devel \
                 libXcursor-devel libXi-devel mesa-libGL-devel

# Arch:
sudo pacman -S libx11 libxrandr libxinerama libxcursor libxi mesa
```

### "CMake Error: CMAKE_CXX_COMPILER not set"

Old cached generator settings. Clean and reconfigure:

```bash
rm -rf build/debug
cmake --preset debug
```

### LuaJIT not found (Linux)

```bash
# Try system package first
# Debian/Ubuntu:
sudo apt install libluajit-5.1-dev

# Or use vcpkg
vcpkg install luajit
```

### Build is slow

Enable unity builds:

```bash
cmake --preset debug -DARTIFACT_USE_UNITY_BUILD=ON
cmake --build --preset debug-build -j$(nproc)  # or -j8, etc.
```

---

## IDE Integration

### Visual Studio

1. Open folder in VS: `File > Open > Folder...`
2. Select preset from dropdown: `debug`, `release`, etc.
3. Build: `Build > Build All`

### CLion

1. Open project
2. Go to `Settings > Build, Execution, Deployment > CMake`
3. CLion auto-detects presets
4. Select preset from toolbar

### VSCode

1. Install **CMake Tools** extension
2. Open folder
3. Run: `CMake: Select Configure Preset` → choose preset
4. Run: `CMake: Build`

---

## Performance Tips

1. **Use Ninja** (faster than Make/VS):
   ```bash
   cmake --preset debug -G Ninja
   ```

2. **Parallel builds**:
   ```bash
   cmake --build --preset debug-build -j8
   ```

3. **Unity builds** (trade-off: faster but less incremental):
   ```bash
   cmake --preset debug -DARTIFACT_USE_UNITY_BUILD=ON
   ```

4. **ccache** (Linux/macOS):
   ```bash
   sudo apt install ccache  # or brew install ccache
   cmake --preset debug -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
   ```

---

## Advanced: Building Without vcpkg

If you prefer system packages (not recommended):

```bash
# Don't set VCPKG_ROOT
unset VCPKG_ROOT

# Install all dependencies via system package manager
# (You'll need to install all packages from vcpkg.json manually)

# Configure
cmake --preset debug
```

This is **much more work** and **less portable** than using vcpkg.

---

## Getting Help

- Check this document first
- Review error logs in `build/<preset>/`
- Check CMake output for missing dependencies
- Ensure vcpkg is up to date
- Try with a clean build directory

For project-specific issues, see the main README.md.

