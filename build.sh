#!/bin/bash
# Build script for Artifact Engine on Linux/macOS
# Usage: ./build.sh [debug|release|relwithdebinfo] [configure|build|rebuild|clean]

set -e

# Default values
BUILD_TYPE=${1:-debug}
ACTION=${2:-build}

# Convert to lowercase
BUILD_TYPE=$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')
ACTION=$(echo "$ACTION" | tr '[:upper:]' '[:lower:]')

# Determine platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
else
    PLATFORM="linux"
fi

# Set preset name
PRESET="${PLATFORM}-${BUILD_TYPE}"

echo "========================================"
echo "Artifact Engine Build Script"
echo "========================================"
echo "Platform: ${PLATFORM}"
echo "Build Type: ${BUILD_TYPE}"
echo "Action: ${ACTION}"
echo "Preset: ${PRESET}"
echo "========================================"
echo ""

# Check dependencies on Linux
if [[ "$PLATFORM" == "linux" ]]; then
    if ! command -v pkg-config &> /dev/null; then
        echo "WARNING: pkg-config not found. Some dependencies may not be located."
        echo "Install with: sudo apt-get install pkg-config"
    fi
fi

# Execute action
case "$ACTION" in
    configure)
        echo "Configuring project with preset: ${PRESET}"
        cmake --preset "${PRESET}"
        echo "Configuration successful!"
        ;;
    build)
        echo "Building project with preset: ${PRESET}"
        cmake --build --preset "${PRESET}-build"
        echo "Build successful!"
        ;;
    rebuild)
        echo "Rebuilding project with preset: ${PRESET}"
        cmake --preset "${PRESET}"
        cmake --build --preset "${PRESET}-build"
        echo "Rebuild successful!"
        ;;
    clean)
        echo "Cleaning build directory: build/${PRESET}"
        if [ -d "build/${PRESET}" ]; then
            rm -rf "build/${PRESET}"
            echo "Clean successful!"
        else
            echo "Build directory does not exist."
        fi
        ;;
    *)
        echo "Unknown action: ${ACTION}"
        echo "Valid actions: configure, build, rebuild, clean"
        exit 1
        ;;
esac

echo ""
echo "========================================"
echo "Done!"
echo "========================================"
