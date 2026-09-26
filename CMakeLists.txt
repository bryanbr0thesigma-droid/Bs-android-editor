# Android/Quest build for the native mod (.so). This is cross-compiled with
# the Android NDK against headers restored by `qpm restore` into extern/ —
# it is NOT built by this repo's CI sandbox and is not meant to be run with
# a host compiler. For a build that runs anywhere with just cmake+g++, see
# tests/CMakeLists.txt, which builds only the engine-agnostic core library.
#
# Expected local build flow (see README.md for full setup):
#   qpm restore
#   ./build.sh
#   qpm qmod build   (or your qpm version's qmod-packaging command)

cmake_minimum_required(VERSION 3.21)
project(bs_android_editor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED MOD_ID)
    set(MOD_ID "bs-android-editor")
endif()
if(NOT DEFINED MOD_VERSION)
    set(MOD_VERSION "0.1.0")
endif()

# Populated by `qpm restore`: extern/includes has every dependency's public
# headers (beatsaber-hook, bs-cordl codegen, bsml, ...), extern/libs has
# their prebuilt static/shared libraries for arm64-v8a.
include_directories(${CMAKE_SOURCE_DIR}/extern/includes)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/libs)

file(GLOB_RECURSE bs_editor_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_library(${MOD_ID} SHARED ${bs_editor_sources})

target_compile_definitions(${MOD_ID} PRIVATE
    VERSION="${MOD_VERSION}"
    MOD_ID="${MOD_ID}"
)

target_compile_options(${MOD_ID} PRIVATE
    -Wall
    -Wextra
    -fexceptions
    -frtti
)

target_link_directories(${MOD_ID} PRIVATE ${CMAKE_SOURCE_DIR}/extern/libs)

file(GLOB restored_shared_libs ${CMAKE_SOURCE_DIR}/extern/libs/*.so)
file(GLOB restored_static_libs ${CMAKE_SOURCE_DIR}/extern/libs/*.a)

target_link_libraries(${MOD_ID} PRIVATE
    ${restored_shared_libs}
    ${restored_static_libs}
    log
)
