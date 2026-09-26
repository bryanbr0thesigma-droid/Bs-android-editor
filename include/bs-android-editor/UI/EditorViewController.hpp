#pragma once

// The body of the "Map Editor" menu screen: a song list plus New/Edit
// buttons, laid out from resources/EditorMain.bsml (inlined in the .cpp).
//
// Verified against bsml 0.4.55 (the version this project's dependencies
// resolve to for Beat Saber 1.40.8_7379 — see qpm.json's bs-cordl pin) and
// custom-types (Il2CppQuestTypePatching) macros.hpp:
//   - DECLARE_CLASS_CODEGEN(ns, name, base) expands to a class HEADER only
//     (`class ns::name : public ...`) — the body must be written as an
//     ordinary `{ ... };` block after the macro call, not passed as macro
//     arguments.
//   - Every DECLARE_CLASS_CODEGEN needs a matching DEFINE_TYPE(ns, name) in
//     exactly one .cpp, which provides the out-of-line storage the
//     DECLARE macros only forward-declare; omitting it is a link error.
//   - bsml 0.4.55 has no BSML::BSMLViewController convenience base (that
//     was added later) — every raw view controller in bsml's own source at
//     this version (e.g. BSML::MenuButtonsViewController) derives directly
//     from HMUI::ViewController, overrides DidActivate itself via
//     DECLARE_OVERRIDE_METHOD_MATCH, and calls the static
//     BSML::BSMLParser::parse_and_construct(str, parent, host) by hand on
//     first activation. This class follows that same pattern.
//   - BSML markup's on-click/select-cell attributes are wired by
//     BSML::BSMLAction, which resolves a MethodInfo* by method name, so the
//     click/selection handlers below also need DECLARE_INSTANCE_METHOD,
//     not just a plain C++ member function.
#include "custom-types/shared/macros.hpp"

#include "HMUI/TableView.hpp"
#include "HMUI/ViewController.hpp"
#include "bsml/shared/BSML/Components/CustomListTableData.hpp"

DECLARE_CLASS_CODEGEN(BSAndroidEditor, EditorViewController, HMUI::ViewController) {
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::ViewController::DidActivate, bool firstActivation,
                                  bool addedToHierarchy, bool screenSystemEnabling);

    // Exact parameter list for OnSongSelected should be checked against
    // BSML's ListTag/CustomListTableDataHandler select-cell wiring once
    // song-list population (still a TODO in the .cpp) is implemented;
    // a mismatch here is a runtime wiring issue, not a compile error,
    // since BSML resolves these by name+argc at runtime.
    DECLARE_INSTANCE_METHOD(void, OnSongSelected, HMUI::TableView* tableView, int index);
    DECLARE_INSTANCE_METHOD(void, OnNewBlankMapClicked);
    DECLARE_INSTANCE_METHOD(void, OnEditSelectedClicked);

    DECLARE_INSTANCE_FIELD(BSML::CustomListTableData*, songList);

  public:
    int selectedSongIndex = -1;
};
