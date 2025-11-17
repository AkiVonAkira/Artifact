# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Projects/Unsorted/Artifact/build/windows-debug/_deps/luajit_source-src")
  file(MAKE_DIRECTORY "C:/Projects/Unsorted/Artifact/build/windows-debug/_deps/luajit_source-src")
endif()
file(MAKE_DIRECTORY
  "C:/Projects/Unsorted/Artifact/build/windows-debug/_deps/luajit_source-build"
  "C:/Projects/Unsorted/Artifact/build/windows-debug/luajit_source-subbuild/luajit_source-populate-prefix"
  "C:/Projects/Unsorted/Artifact/build/windows-debug/luajit_source-subbuild/luajit_source-populate-prefix/tmp"
  "C:/Projects/Unsorted/Artifact/build/windows-debug/luajit_source-subbuild/luajit_source-populate-prefix/src/luajit_source-populate-stamp"
  "C:/Projects/Unsorted/Artifact/build/windows-debug/luajit_source-subbuild/luajit_source-populate-prefix/src"
  "C:/Projects/Unsorted/Artifact/build/windows-debug/luajit_source-subbuild/luajit_source-populate-prefix/src/luajit_source-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Projects/Unsorted/Artifact/build/windows-debug/luajit_source-subbuild/luajit_source-populate-prefix/src/luajit_source-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Projects/Unsorted/Artifact/build/windows-debug/luajit_source-subbuild/luajit_source-populate-prefix/src/luajit_source-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
