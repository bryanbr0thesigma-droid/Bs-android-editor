#pragma once

namespace bs_editor::hooks {

// Installs the hook that, once per frame while an EditorSession is active,
// reads VR controller state and feeds it into EditorController::ProcessFrame.
// Call once from the mod's load entry point (see src/main.cpp).
void InstallGameplayHooks();

} // namespace bs_editor::hooks
