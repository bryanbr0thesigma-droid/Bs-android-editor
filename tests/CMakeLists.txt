# Standalone host build for the engine-agnostic core library's test suite.
# Deliberately independent of the Android/QPM build (../CMakeLists.txt):
# no NDK, no beatsaber-hook, no codegen required. Build with:
#
#   cmake -S tests -B build-tests
#   cmake --build build-tests
#   ctest --test-dir build-tests --output-on-failure

cmake_minimum_required(VERSION 3.16)
project(bs_editor_core_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BS_EDITOR_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(bs_editor_core_tests
    ${BS_EDITOR_ROOT}/src/Core/GridMath.cpp
    ${BS_EDITOR_ROOT}/src/Core/BeatmapSerializer.cpp
    ${BS_EDITOR_ROOT}/src/Core/EditorDocument.cpp
    ${BS_EDITOR_ROOT}/src/Core/SongImport.cpp
    ${BS_EDITOR_ROOT}/src/EditorController.cpp
    ${BS_EDITOR_ROOT}/src/EditorSession.cpp
    test_main.cpp
)

target_include_directories(bs_editor_core_tests PRIVATE
    ${BS_EDITOR_ROOT}/include
    ${BS_EDITOR_ROOT}/libs
)

target_compile_options(bs_editor_core_tests PRIVATE -Wall -Wextra)

enable_testing()
add_test(NAME core_tests COMMAND bs_editor_core_tests)
