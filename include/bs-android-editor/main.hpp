#pragma once

// paper2_scotland2's logging replaced beatsaber-hook's old built-in Logger
// class as of beatsaber-hook ~7.x/8.x; MOD_ID is supplied by CMakeLists.txt
// (target_compile_definitions) as a string literal, consistently across
// every translation unit, so this inline variable is one shared instance
// with no ODR issues.
#include "paper2_scotland2/shared/logger.hpp"

inline constexpr auto Logger = Paper::ConstLoggerContext(MOD_ID);
