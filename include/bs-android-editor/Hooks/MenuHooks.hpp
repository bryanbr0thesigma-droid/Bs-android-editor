#pragma once

namespace bs_editor::hooks {

// Installs the hook that adds a "Map Editor" button to Beat Saber's main
// menu. Call once from the mod's load entry point (see src/main.cpp).
void InstallMenuHooks();

} // namespace bs_editor::hooks
