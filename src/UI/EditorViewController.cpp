#include "bs-android-editor/UI/EditorViewController.hpp"
#include "bs-android-editor/EditorSession.hpp"
#include "bs-android-editor/Hooks/EditorLauncher.hpp"
#include "bs-android-editor/main.hpp"

#include "songcore/shared/SongCore.hpp"

#include "bsml/shared/BSML/Components/CustomListTableData.hpp"
#include "bsml/shared/BSML/Parsing/BSMLParser.hpp"

DEFINE_TYPE(BSAndroidEditor, EditorViewController);

using namespace BSAndroidEditor;

namespace {

// Kept in sync with resources/EditorMain.bsml by hand; see that file's
// header comment for how to switch to loading it at runtime instead.
//
// "Edit Selected" is always interactable (not gated on selectedSongIndex
// via the markup) because there's no BSML-side "enable this button when
// something's selected" wiring done here yet - OnEditSelectedClicked
// checks selectedSongIndex itself and logs instead of doing nothing.
constexpr auto kEditorMainLayout = R"bsml(
<vertical child-control-height="false" child-expand-height="false" spacing="1">
  <horizontal pad="1">
    <text text="Map Editor" align="Center" font-size="6"/>
  </horizontal>
  <list id="song-list" select-cell="OnSongSelected" expand-cell="true" list-width="90"/>
  <horizontal pad="2" spacing="2">
    <button text="New Blank Map" on-click="OnNewBlankMapClicked"/>
    <button text="Edit Selected" id="edit-button" on-click="OnEditSelectedClicked"/>
  </horizontal>
</vertical>
)bsml";

} // namespace

void EditorViewController::DidActivate(bool firstActivation, bool /*addedToHierarchy*/,
                                        bool /*screenSystemEnabling*/) {
    if (firstActivation) {
        BSML::BSMLParser::parse_and_construct(kEditorMainLayout, get_transform(), this);
    }
    RefreshSongList();
}

void EditorViewController::RefreshSongList() {
    levels_.clear();
    for (auto* level : SongCore::API::Loading::GetAllLevels()) {
        levels_.push_back(level);
    }
    selectedSongIndex = -1;

    if (songList == nullptr) return; // not parsed yet (shouldn't happen after firstActivation)

    songList->data.clear();
    for (auto* level : levels_) {
        songList->data.push_back(BSML::CustomCellInfo::construct(level->songName));
    }
    songList->tableView->ReloadData();
}

void EditorViewController::OnSongSelected(HMUI::TableView* /*tableView*/, int index) {
    selectedSongIndex = index;
}

void EditorViewController::OnNewBlankMapClicked() {
    if (selectedSongIndex < 0 || selectedSongIndex >= static_cast<int>(levels_.size())) {
        Logger.info("OnNewBlankMapClicked: select a song first (New Blank Map still needs an existing song's "
                    "audio - see README, song import isn't implemented yet)");
        return;
    }
    bs_editor::hooks::StartEditingLevel(levels_[selectedSongIndex], /*blank=*/true);
}

void EditorViewController::OnEditSelectedClicked() {
    if (selectedSongIndex < 0 || selectedSongIndex >= static_cast<int>(levels_.size())) {
        Logger.info("OnEditSelectedClicked: no song selected");
        return;
    }
    bs_editor::hooks::StartEditingLevel(levels_[selectedSongIndex], /*blank=*/false);
}
