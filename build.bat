@echo off
REM Build script for Artifact Engine on Windows
REM Usage: build.bat [debug|release|relwithdebinfo] [configure|build|rebuild|clean]

setlocal enabledelayedexpansion

REM Default values
set BUILD_TYPE=debug
set ACTION=build

REM Parse arguments
if not "%~1"=="" set BUILD_TYPE=%~1
if not "%~2"=="" set ACTION=%~2

REM Convert to proper case
if /i "%BUILD_TYPE%"=="debug" set BUILD_TYPE=debug
if /i "%BUILD_TYPE%"=="release" set BUILD_TYPE=release
if /i "%BUILD_TYPE%"=="relwithdebinfo" set BUILD_TYPE=relwithdebinfo

REM Set preset name
set PRESET=windows-%BUILD_TYPE%

echo ========================================
echo Artifact Engine Build Script
echo ========================================
echo Build Type: %BUILD_TYPE%
echo Action: %ACTION%
echo Preset: %PRESET%
echo ========================================
echo.

REM Check if VCPKG_ROOT is set
if not defined VCPKG_ROOT (
    echo ERROR: VCPKG_ROOT environment variable is not set!
    echo Please set VCPKG_ROOT to your vcpkg installation directory.
    echo Example: set VCPKG_ROOT=C:\vcpkg
    exit /b 1
)

echo Using vcpkg from: %VCPKG_ROOT%
echo.

REM Execute action
if /i "%ACTION%"=="configure" (
    echo Configuring project with preset: %PRESET%
    cmake --preset %PRESET%
    if errorlevel 1 (
        echo Configuration failed!
        exit /b 1
    )
    echo Configuration successful!
) else if /i "%ACTION%"=="build" (
    echo Building project with preset: %PRESET%
    cmake --build --preset %PRESET%-build
    if errorlevel 1 (
        echo Build failed!
        exit /b 1
    )
    echo Build successful!
) else if /i "%ACTION%"=="rebuild" (
    echo Rebuilding project with preset: %PRESET%
    cmake --preset %PRESET%
    if errorlevel 1 (
        echo Configuration failed!
        exit /b 1
    )
    cmake --build --preset %PRESET%-build
    if errorlevel 1 (
        echo Build failed!
        exit /b 1
    )
    echo Rebuild successful!
) else if /i "%ACTION%"=="clean" (
    echo Cleaning build directory: build\%PRESET%
    if exist "build\%PRESET%" (
        rd /s /q "build\%PRESET%"
        echo Clean successful!
    ) else (
        echo Build directory does not exist.
    )
) else (
    echo Unknown action: %ACTION%
    echo Valid actions: configure, build, rebuild, clean
    exit /b 1
)

echo.
echo ========================================
echo Done!
echo ========================================
