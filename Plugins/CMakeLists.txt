# Plugins

cmake_minimum_required(VERSION 3.21)

# Add subdirectories only if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Core/CMakeLists.txt")
    add_subdirectory(Core)
else()
    message(STATUS "Plugins/Core subdirectory not found - skipping")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SplineShooter/CMakeLists.txt")
    add_subdirectory(SplineShooter)
else()
    message(STATUS "Plugins/SplineShooter subdirectory not found - skipping")
endif()
