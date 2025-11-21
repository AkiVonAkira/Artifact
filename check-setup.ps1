# Artifact CMake Configuration Checker
# This script helps diagnose common CMake/IDE setup issues on Windows

Write-Host "=== Artifact Engine Setup Checker ===" -ForegroundColor Cyan
Write-Host ""

# Check VCPKG_ROOT
Write-Host "Checking VCPKG_ROOT environment variable..." -ForegroundColor Yellow
$vcpkgRoot = $env:VCPKG_ROOT
if ($vcpkgRoot) {
    Write-Host "✓ VCPKG_ROOT is set to: $vcpkgRoot" -ForegroundColor Green
    if (Test-Path $vcpkgRoot) {
        Write-Host "✓ Path exists" -ForegroundColor Green
        $toolchainPath = Join-Path $vcpkgRoot "scripts\buildsystems\vcpkg.cmake"
        if (Test-Path $toolchainPath) {
            Write-Host "✓ vcpkg toolchain file found" -ForegroundColor Green
        } else {
            Write-Host "✗ vcpkg toolchain file NOT found at: $toolchainPath" -ForegroundColor Red
            Write-Host "  Run 'git clone https://github.com/Microsoft/vcpkg.git' and bootstrap vcpkg" -ForegroundColor Yellow
        }
    } else {
        Write-Host "✗ Path does NOT exist: $vcpkgRoot" -ForegroundColor Red
        Write-Host "  Set VCPKG_ROOT to a valid vcpkg installation path" -ForegroundColor Yellow
    }
} else {
    Write-Host "✗ VCPKG_ROOT is NOT set" -ForegroundColor Red
    Write-Host "  Set it with:" -ForegroundColor Yellow
    Write-Host '  [System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")' -ForegroundColor Gray
}
Write-Host ""

# Check CMake
Write-Host "Checking CMake installation..." -ForegroundColor Yellow
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmake) {
    $cmakeVersion = & cmake --version | Select-Object -First 1
    Write-Host "✓ CMake found: $cmakeVersion" -ForegroundColor Green
    Write-Host "  Location: $($cmake.Source)" -ForegroundColor Gray
} else {
    Write-Host "✗ CMake NOT found" -ForegroundColor Red
    Write-Host "  Install via: winget install Kitware.CMake" -ForegroundColor Yellow
}
Write-Host ""

# Check Ninja
Write-Host "Checking Ninja build system..." -ForegroundColor Yellow
$ninja = Get-Command ninja -ErrorAction SilentlyContinue
if ($ninja) {
    $ninjaVersion = & ninja --version
    Write-Host "✓ Ninja found: v$ninjaVersion" -ForegroundColor Green
    Write-Host "  Location: $($ninja.Source)" -ForegroundColor Gray
} else {
    Write-Host "✗ Ninja NOT found (required for current presets)" -ForegroundColor Red
    Write-Host "  Install via: winget install Ninja-build.Ninja" -ForegroundColor Yellow
}
Write-Host ""

# Check Visual Studio
Write-Host "Checking Visual Studio installation..." -ForegroundColor Yellow
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $vsInstances = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vsInstances) {
        Write-Host "✓ Visual Studio 2022 with C++ tools found" -ForegroundColor Green
        Write-Host "  Location: $vsInstances" -ForegroundColor Gray

        # Check for MSVC
        $msvcPath = Join-Path $vsInstances "VC\Tools\MSVC"
        if (Test-Path $msvcPath) {
            $msvcVersions = Get-ChildItem $msvcPath | Sort-Object Name -Descending | Select-Object -First 1
            Write-Host "  MSVC version: $($msvcVersions.Name)" -ForegroundColor Gray
            Write-Host "  (Used by Ninja generator via CMake presets)" -ForegroundColor Gray
        }
    } else {
        Write-Host "✗ Visual Studio with C++ tools NOT found" -ForegroundColor Red
        Write-Host "  Install Visual Studio 2022 Community with 'Desktop development with C++' workload" -ForegroundColor Yellow
    }
} else {
    Write-Host "⚠ Visual Studio installer not found" -ForegroundColor Yellow
    Write-Host "  Install Visual Studio 2022 Community from: https://visualstudio.microsoft.com/" -ForegroundColor Yellow
}
Write-Host ""

# Check Git
Write-Host "Checking Git installation..." -ForegroundColor Yellow
$git = Get-Command git -ErrorAction SilentlyContinue
if ($git) {
    $gitVersion = & git --version
    Write-Host "✓ Git found: $gitVersion" -ForegroundColor Green
} else {
    Write-Host "✗ Git NOT found" -ForegroundColor Red
    Write-Host "  Install via: winget install Git.Git" -ForegroundColor Yellow
}
Write-Host ""

# Check project structure
Write-Host "Checking Artifact project structure..." -ForegroundColor Yellow
$currentPath = Get-Location
$cmakePresetsPath = Join-Path $currentPath "CMakePresets.json"
$cmakeListsPath = Join-Path $currentPath "CMakeLists.txt"

if (Test-Path $cmakePresetsPath) {
    Write-Host "✓ CMakePresets.json found" -ForegroundColor Green
} else {
    Write-Host "✗ CMakePresets.json NOT found" -ForegroundColor Red
    Write-Host "  Are you in the Artifact project root directory?" -ForegroundColor Yellow
}

if (Test-Path $cmakeListsPath) {
    Write-Host "✓ CMakeLists.txt found" -ForegroundColor Green
} else {
    Write-Host "✗ CMakeLists.txt NOT found" -ForegroundColor Red
    Write-Host "  Are you in the Artifact project root directory?" -ForegroundColor Yellow
}
Write-Host ""

# Summary and recommendations
Write-Host "=== Recommendations ===" -ForegroundColor Cyan
Write-Host ""

$allGood = $vcpkgRoot -and (Test-Path $vcpkgRoot) -and $cmake

if ($allGood) {
    Write-Host "✓ Your system is ready to build Artifact!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "1. Configure with Ninja preset:" -ForegroundColor White
    Write-Host "   cmake --preset Win64-Editor-Debug" -ForegroundColor Gray
    Write-Host "2. Build:" -ForegroundColor White
    Write-Host "   cmake --build --preset Build-Win64-Editor-Debug" -ForegroundColor Gray
    Write-Host ""
    Write-Host "See docs/IDE_SETUP.md for detailed IDE-specific instructions" -ForegroundColor Cyan
} else {
    Write-Host "⚠ Please address the issues marked with ✗ above" -ForegroundColor Yellow
    Write-Host ""
    if (-not $vcpkgRoot -or -not (Test-Path $vcpkgRoot)) {
        Write-Host "Priority: Set up vcpkg" -ForegroundColor Red
        Write-Host "1. Clone vcpkg: git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg" -ForegroundColor Gray
        Write-Host "2. Bootstrap: cd C:\vcpkg; .\bootstrap-vcpkg.bat" -ForegroundColor Gray
        Write-Host "3. Set environment variable:" -ForegroundColor Gray
        Write-Host '   [System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")' -ForegroundColor Gray
        Write-Host "4. Restart your terminal/IDE" -ForegroundColor Gray
    }
    if (-not $cmake) {
        Write-Host "Priority: Install CMake" -ForegroundColor Red
        Write-Host "   winget install Kitware.CMake" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "For more help, see docs/IDE_SETUP.md" -ForegroundColor Cyan
