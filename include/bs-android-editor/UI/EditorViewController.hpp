#pragma once

// The body of the "Map Editor" menu screen: a song list plus New/Edit
// buttons, laid out from resources/EditorMain.bsml (inlined in the .cpp).
//
// VERSION-SENSITIVE FILE: uses the classic `custom-types` idiom
// (DECLARE_CLASS_CODEGEN/DEFINE_TYPE/DECLARE_OVERRIDE_METHOD_MATCH) that has
// been the standard way to register a new IL2CPP-visible UI class for years
// of Quest modding (see the BSMG wiki's "Creating a Menu" guide and
// custom-types' own README/examples for the canonical, currently-correct
// spelling for your installed versions). If your restored `custom-types`/
// `bsml` expose newer convenience macros, prefer those — what matters is
// *what* this class does: parse the layout, list installed custom levels,
// and start an EditorSession on selection.
#include "custom-types/shared/macros.hpp"

#include "HMUI/TableView.hpp"
#include "HMUI/ViewController.hpp"
#include "bsml/shared/BSML/Components/CustomListTableData.hpp"

DECLARE_CLASS_CODEGEN(BSAndroidEditor, EditorViewController, HMUI::ViewController,
    DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate, &HMUI::ViewController::DidActivate,
                                  bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);

    DECLARE_INSTANCE_FIELD(BSML::CustomListTableData*, songList);

  public:
    int selectedSongIndex = -1;

    void OnSongSelected(HMUI::TableView* tableView, int index);
    void OnNewBlankMapClicked();
    void OnEditSelectedClicked();
)
