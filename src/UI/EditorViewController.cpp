#include "bs-android-editor/UI/EditorViewController.hpp"
#include "bs-android-editor/EditorSession.hpp"
#include "bs-android-editor/Hooks/EditorLauncher.hpp"
#include "bs-android-editor/main.hpp"

#include "songcore/shared/SongCore.hpp"

#include "bsml/shared/BSML-Lite/Creation/Buttons.hpp"
#include "bsml/shared/BSML-Lite/Creation/Settings.hpp"
#include "bsml/shared/BSML/Components/CustomListTableData.hpp"
#include "bsml/shared/BSML/Parsing/BSMLParser.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Vector2.hpp"

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
    // Reopening this screen (even just via the back button) always resets
    // back to the normal "edit an existing song" list - see the header
    // comment for why that's the only way out of import mode.
    importMode_ = false;
    if (bpmSetting_ != nullptr) bpmSetting_->get_gameObject()->SetActive(false);
    if (nameSetting_ != nullptr) nameSetting_->get_gameObject()->SetActive(false);
    if (createNewSongButton_ != nullptr) createNewSongButton_->get_gameObject()->SetActive(false);
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

void EditorViewController::CreateImportControlsIfNeeded() {
    if (bpmSetting_ != nullptr) return; // already created, just toggled active/inactive from here on

    // Parented directly to this screen's own transform, same as the main
    // menu button in MenuHooks.cpp - these end up as siblings of the
    // parsed <vertical> layout above, not children of it, positioned by
    // their own anchoredPosition. The values below are a first guess, not
    // measured against a running game; nudge them if they land somewhere
    // awkward.
    nameSetting_ = BSML::Lite::CreateStringSetting(get_transform(), "Song Name", "", UnityEngine::Vector2(0, -15),
                                                    [this](StringW value) { pendingSongName_ = std::string(value); });
    bpmSetting_ = BSML::Lite::CreateIncrementSetting(get_transform(), "BPM", /*decimals=*/1, /*increment=*/1.0f,
                                                      /*currentValue=*/static_cast<float>(pendingBpm_),
                                                      /*minValue=*/40.0f, /*maxValue=*/400.0f,
                                                      UnityEngine::Vector2(0, -30),
                                                      [this](float value) { pendingBpm_ = value; });
    createNewSongButton_ = BSML::Lite::CreateUIButton(get_transform(), "Create & Edit New Song",
                                                       UnityEngine::Vector2(0, -45),
                                                       [this] { OnCreateNewSongClicked(); });
}

void EditorViewController::EnterImportMode() {
    if (!importMode_) {
        importMode_ = true;
        pendingSongName_.clear();
        pendingBpm_ = 120.0;
    }
    CreateImportControlsIfNeeded();
    bpmSetting_->get_gameObject()->SetActive(true);
    nameSetting_->get_gameObject()->SetActive(true);
    createNewSongButton_->get_gameObject()->SetActive(true);
    RefreshImportList();
}

void EditorViewController::RefreshImportList() {
    importFiles_ = bs_editor::hooks::ListImportableAudioFiles();
    selectedSongIndex = -1;

    if (songList == nullptr) return;

    songList->data.clear();
    if (importFiles_.empty()) {
        // Otherwise an empty folder just looks like the list/picker is
        // missing entirely, with no indication of why - this was the
        // actual bug behind the first real-headset report of this
        // feature ("no audio picker"), alongside the import folder only
        // having been created lazily (see main.cpp's load()).
        songList->data.push_back(BSML::CustomCellInfo::construct(
            "No .ogg/.egg files found - see README for the import folder path"));
    } else {
        for (const auto& file : importFiles_) {
            songList->data.push_back(BSML::CustomCellInfo::construct(file.filename().string()));
        }
    }
    songList->tableView->ReloadData();

    Logger.info("RefreshImportList: found {} importable audio file(s) in {}", importFiles_.size(),
                bs_editor::hooks::GetImportAudioDirectory().string());
}

void EditorViewController::OnSongSelected(HMUI::TableView* /*tableView*/, int index) {
    selectedSongIndex = index;
}

void EditorViewController::OnNewBlankMapClicked() {
    // Re-clicking while already in import mode just rescans the import
    // folder, so dropping in a new file doesn't need leaving this screen.
    EnterImportMode();
}

void EditorViewController::OnEditSelectedClicked() {
    if (importMode_) {
        Logger.info("OnEditSelectedClicked: in import mode - use 'Create & Edit New Song' instead");
        return;
    }

    if (selectedSongIndex < 0 || selectedSongIndex >= static_cast<int>(levels_.size())) {
        Logger.info("OnEditSelectedClicked: no song selected");
        return;
    }
    bs_editor::hooks::StartEditingLevel(levels_[selectedSongIndex], /*blank=*/false);
}

void EditorViewController::OnCreateNewSongClicked() {
    if (selectedSongIndex < 0 || selectedSongIndex >= static_cast<int>(importFiles_.size())) {
        Logger.info("OnCreateNewSongClicked: select an audio file first");
        return;
    }

    const auto& audioFile = importFiles_[selectedSongIndex];
    std::string songName = pendingSongName_.empty() ? audioFile.stem().string() : pendingSongName_;
    Logger.info("OnCreateNewSongClicked: importing '{}' as '{}' at {} BPM", audioFile.string(), songName, pendingBpm_);
    bs_editor::hooks::CreateNewSong(audioFile, songName, pendingBpm_);
}
