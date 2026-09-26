#!/usr/bin/env bash
# Cross-compiles the mod's .so for arm64-v8a using the Android NDK.
# Requires: qpm restore has already been run (extern/, qpm_defines.cmake and
# extern.cmake all present). qpm_defines.cmake picks up the NDK itself from
# ANDROID_NDK_HOME / ANDROID_NDK_LATEST_HOME (or an ndkpath.txt file), so
# this script does not pass -DCMAKE_TOOLCHAIN_FILE/-DANDROID_ABI/etc. by
# hand — those would just be shadowed by qpm_defines.cmake's own set()
# calls anyway.
set -euo pipefail

if [[ ! -d extern || ! -f qpm_defines.cmake || ! -f extern.cmake ]]; then
    echo "error: extern/, qpm_defines.cmake or extern.cmake not found — run 'qpm restore' first" >&2
    exit 1
fi

if [[ -z "${ANDROID_NDK_HOME:-}" && -z "${ANDROID_NDK_LATEST_HOME:-}" ]]; then
    echo "error: set ANDROID_NDK_HOME (or ANDROID_NDK_LATEST_HOME) to your Android NDK path" >&2
    exit 1
fi

BUILD_DIR="build"

cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build "${BUILD_DIR}" -- -j"$(nproc)"

echo ""
echo "Build complete — the compiled .so is under ${BUILD_DIR}/."
echo "Next: package it into a .qmod with 'qpm qmod zip' (not 'qpm qmod build' — that's a deprecated"
echo "alias for regenerating mod.json only, per qpm.cli's own src/commands/qmod/mod.rs)."
