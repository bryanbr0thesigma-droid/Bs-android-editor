#include "bs-android-editor/UI/EditorViewController.hpp"
#include "bs-android-editor/EditorSession.hpp"
#include "bs-android-editor/main.hpp"

#include "bsml/shared/BSML/Parsing/BSMLParser.hpp"

DEFINE_TYPE(BSAndroidEditor, EditorViewController);

using namespace BSAndroidEditor;

namespace {

// Kept in sync with resources/EditorMain.bsml by hand; see that file's
// header comment for how to switch to loading it at runtime instead.
constexpr auto kEditorMainLayout = R"bsml(
<vertical child-control-height="false" child-expand-height="false" spacing="1">
  <horizontal pad="1">
    <text text="Map Editor" align="Center" font-size="6"/>
  </horizontal>
  <list id="song-list" select-cell="OnSongSelected" expand-cell="true" list-width="90"/>
  <horizontal pad="2" spacing="2">
    <button text="New Blank Map" on-click="OnNewBlankMapClicked"/>
    <button text="Edit Selected" id="edit-button" interactable="false" on-click="OnEditSelectedClicked"/>
  </horizontal>
</vertical>
)bsml";

} // namespace

void EditorViewController::DidActivate(bool firstActivation, bool /*addedToHierarchy*/,
                                        bool /*screenSystemEnabling*/) {
    if (!firstActivation) return;
    BSML::BSMLParser::parse_and_construct(kEditorMainLayout, get_transform(), this);
}

void EditorViewController::OnSongSelected(HMUI::TableView* /*tableView*/, int index) {
    selectedSongIndex = index;
}

void EditorViewController::OnNewBlankMapClicked() {
    bs_editor::core::SongInfo info;
    info.songName = "New Song";
    info.beatsPerMinute = 120.0;

    bs_editor::core::BeatmapDifficulty difficulty;
    bs_editor::EditorSession::Instance().Start(info, difficulty, info.beatsPerMinute);

    // TODO: transition into the gameplay scene with the editor session
    // active, typically via GlobalNamespace::MenuTransitionsHelper — this is
    // one of the most frequently reshaped classes across Beat Saber updates,
    // so check its current constructor/method overloads in your
    // extern/includes before wiring this up.
    Logger.info("Started a blank map editor session; gameplay-scene transition is a TODO");
}

void EditorViewController::OnEditSelectedClicked() {
    if (selectedSongIndex < 0) return;

    // TODO: resolve the selected list entry to a song folder, then:
    //   auto info = bs_editor::core::LoadSongInfoFile(infoPath);
    //   auto difficulty = bs_editor::core::LoadDifficultyFile(difficultyPath);
    //   bs_editor::EditorSession::Instance().Start(info, difficulty, info.beatsPerMinute);
    // followed by the same gameplay-scene transition as OnNewBlankMapClicked.
    Logger.info("Edit-selected-song flow is a TODO (wire up SongCore level resolution)");
}
