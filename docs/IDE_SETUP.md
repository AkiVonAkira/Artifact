# IDE Setup Guide

This guide covers configuring the Artifact project on Windows using the provided CMake presets and Ninja generator.

## Prerequisites

1. **vcpkg** installed and `VCPKG_ROOT` set to your installation path (`C:\vcpkg`).
2. **CMake 3.24+** (install via `winget install Kitware.CMake`).
3. **Ninja** build tool (`winget install Ninja-build.Ninja`).
4. **Visual Studio 2022** with the *Desktop development with C++* workload (used to provide the MSVC toolchain invoked by Ninja).

## Configure & Build

```powershell
cmake --preset Win64-Editor-Debug
cmake --build --preset Build-Win64-Editor-Debug
```

For a release-oriented build with Tracy disabled, use `Win64-Editor-RelWithDebInfo` and `Build-Win64-Editor-RelWithDebInfo`.

## IDE Integration

All IDEs should rely on the same presets:

- **CLion**: Open the folder, ensure it respects `CMakePresets.json`, and select `Win64-Editor-Debug` or `Win64-Editor-RelWithDebInfo`.
- **Visual Studio / VS Code**: Use the *CMake Presets* UI to pick the same preset names; no generator changes required.

### Tips

- If presets do not appear, reload the project or run `cmake --preset Win64-Editor-Debug` once in a terminal to prime the cache.
- Keep `build/Windows/Win64-Editor-Debug` out of your IDE’s source indexing to speed up navigation.

## Troubleshooting

- **Missing Ninja:** install it via winget and ensure it is on the PATH before opening your IDE.
- **Toolchain file errors:** confirm `VCPKG_ROOT` is set and restart the IDE after changing environment variables.
- **Compiler not found:** reinstall/repair Visual Studio 2022 C++ components.

## Running Tests

Unit tests remain in the separate `artifact_tests` executable:

```powershell
cmake --build --preset Build-Win64-Editor-Debug --target artifact_tests
ctest --preset Test-Win64-Editor-Debug
```

Gameplay tests register through `ARTIFACT_REGISTER_GAMEPLAY_TEST` and run via the editor’s Gameplay Tests panel.
