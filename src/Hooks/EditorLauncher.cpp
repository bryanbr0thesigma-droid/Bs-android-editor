#include "bs-android-editor/Hooks/EditorLauncher.hpp"
#include "bs-android-editor/Core/BeatmapSerializer.hpp"
#include "bs-android-editor/EditorSession.hpp"
#include "bs-android-editor/main.hpp"

#include "songcore/shared/SongCore.hpp"

#include "beatsaber-hook/shared/utils/byref.hpp"
#include "custom-types/shared/delegate.hpp"

#include "GlobalNamespace/BeatmapKey.hpp"
#include "GlobalNamespace/EnvironmentsListModel.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"
#include "GlobalNamespace/RecordingToolManager.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/StandardLevelScenesTransitionSetupDataSO.hpp"
#include "System/Action.hpp"
#include "System/Action_1.hpp"
#include "System/Action_2.hpp"
#include "System/Nullable_1.hpp"
#include "UnityEngine/Resources.hpp"
#include "Zenject/DiContainer.hpp"

#include <functional>

using namespace GlobalNamespace;

namespace bs_editor::hooks {

namespace {
constexpr const char* kCharacteristic = "Standard";
constexpr const char* kDifficulty = "ExpertPlus";
} // namespace

bool StartEditingLevel(SongCore::SongLoader::CustomBeatmapLevel* level, bool blank) {
    if (level == nullptr) {
        Logger.error("StartEditingLevel: called with a null level");
        return false;
    }

    const std::string levelPath(level->customLevelPath);
    const std::string infoPath = levelPath + "/Info.dat";

    core::SongInfo songInfo;
    try {
        songInfo = core::LoadSongInfoFile(infoPath);
    } catch (const core::BeatmapIOError& e) {
        Logger.error("StartEditingLevel: failed to load '{}': {}", infoPath, e.what());
        return false;
    }

    core::DifficultyBeatmap& slot = core::FindOrAddDifficultySlot(songInfo, kCharacteristic, kDifficulty);
    const std::string difficultyPath = levelPath + "/" + slot.beatmapFilename;

    core::BeatmapDifficulty difficulty;
    if (!blank) {
        try {
            difficulty = core::LoadDifficultyFile(difficultyPath);
        } catch (const core::BeatmapIOError&) {
            // No file yet for this slot (or it's unreadable) - start blank
            // rather than failing; the slot was just created above if it
            // didn't already exist in Info.dat either.
            Logger.info("StartEditingLevel: no existing '{}', starting blank", difficultyPath);
        }
    }

    auto* characteristic = SongCore::API::Characteristics::GetCharacteristicBySerializedName(kCharacteristic);
    if (characteristic == nullptr) {
        Logger.error("StartEditingLevel: couldn't resolve the '{}' characteristic", kCharacteristic);
        return false;
    }

    // MenuTransitionsHelper is reachable the same way BSML's own
    // RestartGame() button grabs it (Resources::FindObjectsOfTypeAll,
    // confirmed against bsml 0.4.55's actual source) rather than through
    // any DI we'd have to hook ourselves. EnvironmentsListModel isn't a
    // MonoBehaviour, so it's read off a flow coordinator that already had
    // it constructor-injected - confirmed field names against bs-cordl
    // 4008.0.0's SinglePlayerLevelSelectionFlowCoordinator header. Both
    // require the player to have opened Solo Play at least once this
    // session (that's when these get created); there is currently no
    // fallback if they haven't.
    auto* menuTransitionsHelper = UnityEngine::Resources::FindObjectsOfTypeAll<MenuTransitionsHelper*>()->FirstOrDefault();
    if (menuTransitionsHelper == nullptr) {
        Logger.error("StartEditingLevel: no MenuTransitionsHelper found yet this session (open Solo Play once first)");
        return false;
    }

    auto* flowCoordinator =
        UnityEngine::Resources::FindObjectsOfTypeAll<SinglePlayerLevelSelectionFlowCoordinator*>()->FirstOrDefault();
    if (flowCoordinator == nullptr) {
        Logger.error("StartEditingLevel: no SinglePlayerLevelSelectionFlowCoordinator found yet this session "
                     "(open Solo Play once first)");
        return false;
    }
    EnvironmentsListModel* environmentsListModel = flowCoordinator->_environmentsListModel;
    if (environmentsListModel == nullptr) {
        Logger.error("StartEditingLevel: flow coordinator has no environmentsListModel yet");
        return false;
    }

    EditorSession::Instance().Start(songInfo, difficulty, songInfo.beatsPerMinute, /*snapSubdivision=*/8, infoPath,
                                     difficultyPath);

    BeatmapKey beatmapKey(characteristic, BeatmapDifficulty::ExpertPlus, level->levelID);

    auto* gameplayModifiers = GameplayModifiers::New_ctor();
    auto* playerSpecificSettings = PlayerSpecificSettings::New_ctor();

    auto afterSceneSwitch = custom_types::MakeDelegate<System::Action_1<Zenject::DiContainer*>*>(
        std::function<void(Zenject::DiContainer*)>([](Zenject::DiContainer*) {
            Logger.info("StartEditingLevel: gameplay scene DiContainer ready");
        }));

    auto levelFinished = custom_types::MakeDelegate<
        System::Action_2<UnityW<StandardLevelScenesTransitionSetupDataSO>, LevelCompletionResults*>*>(
        std::function<void(UnityW<StandardLevelScenesTransitionSetupDataSO>, LevelCompletionResults*)>(
            [](UnityW<StandardLevelScenesTransitionSetupDataSO>, LevelCompletionResults*) {
                Logger.info("StartEditingLevel: gameplay scene finished, ending editor session");
                EditorSession::Instance().End();
            }));

    menuTransitionsHelper->StartStandardLevel(
        "Solo", ByRef(beatmapKey), level,
        /*overrideEnvironmentSettings=*/nullptr,
        /*playerOverrideColorScheme=*/nullptr,
        /*playerOverrideLightshowColors=*/false,
        /*beatmapOverrideColorScheme=*/nullptr, gameplayModifiers, playerSpecificSettings,
        /*practiceSettings=*/nullptr, environmentsListModel,
        /*backButtonText=*/"Menu",
        /*useTestNoteCutSoundEffects=*/false,
        /*startPaused=*/false,
        /*beforeSceneSwitchToGameplayCallback=*/nullptr, afterSceneSwitch, levelFinished,
        /*levelRestartedCallback=*/nullptr, System::Nullable_1<RecordingToolManager_SetupData>());

    return true;
}

} // namespace bs_editor::hooks
