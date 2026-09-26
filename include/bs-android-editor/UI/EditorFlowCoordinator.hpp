#pragma once

// The flow coordinator presented over the main menu when "Map Editor" is
// clicked; owns the (currently single-screen) editor menu flow. See
// EditorViewController.hpp for the same version-sensitivity caveat about
// the custom-types registration macros used here.
#include "custom-types/shared/macros.hpp"
#include "HMUI/FlowCoordinator.hpp"

DECLARE_CLASS_CODEGEN(BSAndroidEditor, EditorFlowCoordinator, HMUI::FlowCoordinator,
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::FlowCoordinator::DidActivate,
                                  bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
)

namespace bs_editor::ui {

// Presents the editor flow coordinator over Beat Saber's current main flow
// coordinator. Call from a main-menu button handler
// (see src/Hooks/MenuHooks.cpp).
void PresentEditorFlow();

} // namespace bs_editor::ui
