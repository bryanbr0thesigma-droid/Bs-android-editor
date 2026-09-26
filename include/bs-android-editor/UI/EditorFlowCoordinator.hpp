#pragma once

// The flow coordinator presented over the main menu when "Map Editor" is
// clicked; owns the (currently single-screen) editor menu flow.
//
// Derives directly from the game's own HMUI::FlowCoordinator codegen
// class and overrides DidActivate itself — the same direct-override
// pattern EditorViewController now also uses against HMUI::ViewController
// (bsml 0.4.55, the version resolved for this project's Beat Saber
// 1.40.8_7379 target, has no BSMLViewController convenience base; see
// EditorViewController.hpp's comment).
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
