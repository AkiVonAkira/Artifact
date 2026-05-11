# Tools

cmake_minimum_required(VERSION 3.21)

if(NOT ARTIFACT_ENABLE_TOOLS)
    message(STATUS "Tools disabled - skipping")
    return()
endif()

# Add subdirectories only if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/AssetPipeline/CMakeLists.txt")
    add_subdirectory(AssetPipeline)
else()
    message(STATUS "Tools/AssetPipeline subdirectory not found - skipping")
endif()

# Optional Benchmarks (timing/experiments)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Benchmarks/CMakeLists.txt")
    add_subdirectory(Benchmarks)
else()
    message(STATUS "Tools/Benchmarks subdirectory not found - skipping")
endif()
