include("C:/Projects/Unsorted/Artifact/build/windows-debug/_deps/tracy-src/cmake/CPM.cmake")
CPMAddPackage("NAME;freetype;GITHUB_REPOSITORY;freetype/freetype;GIT_TAG;VER-2-13-2;OPTIONS;FT_DISABLE_HARFBUZZ ON;FT_WITH_HARFBUZZ OFF")
set(freetype_FOUND TRUE)