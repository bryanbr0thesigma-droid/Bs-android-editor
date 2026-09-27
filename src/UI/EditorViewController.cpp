#include "bs-android-editor/UI/EditorViewController.hpp"
#include "bs-android-editor/EditorSession.hpp"
#include "bs-android-editor/main.hpp"

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

// DIAGNOSTIC BUILD: song-list population (SongCore::API::Loading::
// GetAllLevels()) and the StartStandardLevel scene transition are
// temporarily stubbed out - see the note at the top of
// EditorViewController.hpp and the README's crash-investigation section.
// This screen still opens and its buttons still respond, they just don't
// do anything real yet in this build.
void EditorViewController::DidActivate(bool firstActivation, bool /*addedToHierarchy*/,
                                        bool /*screenSystemEnabling*/) {
    if (firstActivation) {
        BSML::BSMLParser::parse_and_construct(kEditorMainLayout, get_transform(), this);
    }
    selectedSongIndex = -1;
    if (songList != nullptr) {
        songList->data.clear();
        songList->tableView->ReloadData();
    }
}

void EditorViewController::OnSongSelected(HMUI::TableView* /*tableView*/, int index) {
    selectedSongIndex = index;
}

void EditorViewController::OnNewBlankMapClicked() {
    Logger.info("OnNewBlankMapClicked: disabled in this diagnostic build (SongCore integration removed to "
                "isolate the launch crash - see README)");
}

void EditorViewController::OnEditSelectedClicked() {
    Logger.info("OnEditSelectedClicked: disabled in this diagnostic build (SongCore integration removed to "
                "isolate the launch crash - see README)");
}
