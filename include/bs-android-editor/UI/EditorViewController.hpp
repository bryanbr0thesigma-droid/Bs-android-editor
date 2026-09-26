#pragma once

// The body of the "Map Editor" menu screen: a song list plus New/Edit
// buttons, laid out from resources/EditorMain.bsml (inlined in the .cpp).
//
// Verified against bsq-ports/Quest-BSML (BSML::BSMLViewController) and
// custom-types (Il2CppQuestTypePatching) macros.hpp:
//   - DECLARE_CLASS_CODEGEN(ns, name, base) expands to a class HEADER only
//     (`class ns::name : public ...`) — the body must be written as an
//     ordinary `{ ... };` block after the macro call, not passed as macro
//     arguments.
//   - Every DECLARE_CLASS_CODEGEN needs a matching DEFINE_TYPE(ns, name) in
//     exactly one .cpp, which provides the out-of-line storage the
//     DECLARE macros only forward-declare; omitting it is a link error.
//   - Deriving from BSML::BSMLViewController (rather than raw
//     HMUI::ViewController): its DidActivate/ParseWithFallback machinery
//     resolves get_Content()/get_FallbackContent() *by name* on the most-
//     derived registered type at runtime — so this subclass must supply
//     its own get_Content() via a plain DECLARE_INSTANCE_METHOD and must
//     NOT also override DidActivate itself (that would fight the base
//     class's own override of the same virtual slot).
//   - BSML markup's on-click/select-cell attributes are wired the same
//     way (BSML::BSMLAction resolves a MethodInfo* by method name), so the
//     click/selection handlers below also need DECLARE_INSTANCE_METHOD,
//     not just a plain C++ member function.
#include "custom-types/shared/macros.hpp"

#include "HMUI/TableView.hpp"
#include "bsml/shared/BSML/Components/CustomListTableData.hpp"
#include "bsml/shared/BSML/ViewControllers/BSMLViewController.hpp"

DECLARE_CLASS_CODEGEN(BSAndroidEditor, EditorViewController, BSML::BSMLViewController) {
    DECLARE_INSTANCE_METHOD(StringW, get_Content);

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
