#!/usr/bin/env bash
# Cross-compiles the mod's .so for arm64-v8a using the Android NDK.
# Requires: qpm restore has already been run (extern/ populated), and
# ANDROID_NDK_HOME (or ANDROID_NDK_ROOT) points at your NDK install.
set -euo pipefail

NDK_PATH="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
if [[ -z "${NDK_PATH}" ]]; then
    echo "error: set ANDROID_NDK_HOME (or ANDROID_NDK_ROOT) to your Android NDK path" >&2
    exit 1
fi

if [[ ! -d extern ]]; then
    echo "error: extern/ not found — run 'qpm restore' first" >&2
    exit 1
fi

BUILD_DIR="build"
ANDROID_PLATFORM="${ANDROID_PLATFORM:-26}"

cmake -B "${BUILD_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${NDK_PATH}/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM="android-${ANDROID_PLATFORM}" \
    -DCMAKE_BUILD_TYPE=Release \
    "$@"

cmake --build "${BUILD_DIR}" -- -j"$(nproc)"

echo ""
echo "Built: ${BUILD_DIR}/libbs-android-editor.so"
echo "Next: package it into a .qmod (e.g. 'qpm qmod build', or your qpm version's equivalent)."
