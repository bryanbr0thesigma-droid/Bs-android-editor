#pragma once

// The flow coordinator presented over the main menu when "Map Editor" is
// clicked; owns the (currently single-screen) editor menu flow.
//
// Unlike EditorViewController (which derives from BSML's own
// BSMLViewController convenience base and must NOT re-override
// DidActivate — see that header's comment), this class derives directly
// from the game's own HMUI::FlowCoordinator codegen class, so overriding
// DidActivate here is the correct, standard direct-override pattern (the
// same one BSMLViewController itself uses against HMUI::ViewController).
//
// DECLARE_CLASS_CODEGEN(ns, name, base) expands to a class HEADER only —
// the body must follow as an ordinary `{ ... };` block — and needs a
// matching DEFINE_TYPE(ns, name) in the .cpp; see EditorViewController.hpp
// for why (verified against custom-types' macros.hpp).
#include "custom-types/shared/macros.hpp"
#include "HMUI/FlowCoordinator.hpp"

DECLARE_CLASS_CODEGEN(BSAndroidEditor, EditorFlowCoordinator, HMUI::FlowCoordinator) {
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::FlowCoordinator::DidActivate, bool firstActivation,
                                  bool addedToHierarchy, bool screenSystemEnabling);
};

namespace bs_editor::ui {

// Presents the editor flow coordinator over Beat Saber's current main flow
// coordinator. Call from a main-menu button handler
// (see src/Hooks/MenuHooks.cpp).
void PresentEditorFlow();

} // namespace bs_editor::ui
