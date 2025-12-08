# Games

cmake_minimum_required(VERSION 3.21)

# Add subdirectories only if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SplineShooterLab/CMakeLists.txt")
    add_subdirectory(SplineShooterLab)
else()
    message(STATUS "Games/SplineShooterLab subdirectory not found - skipping")
endif()
