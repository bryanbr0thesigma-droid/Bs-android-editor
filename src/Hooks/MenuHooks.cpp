// Injects a "Map Editor" button into the main menu.
//
// Verified against bs-cordl 4008.0.0 (Beat Saber 1.40.8_7379, this
// project's pinned target): MainMenuViewController::DidActivate's
// signature below is the real one for that game version. If you're
// building against a different game version, re-check it in your own
// extern/includes (GlobalNamespace/zzzz__MainMenuViewController_def.hpp)
// — this class stays stable across most updates but isn't guaranteed to.
//
// beatsaber-hook/shared/hooking.hpp lives under a "utils/" subfolder at
// the beatsaber-hook major version (6.4.2) that resolves for this game
// version — it moved to a flat beatsaber-hook/shared/hooking.hpp in
// beatsaber-hook ~7.x+. Same for BSML::Lite::CreateUIButton's location,
// which is still under BSML-Lite/Creation/Buttons.hpp at bsml 0.4.55.
#include "bs-android-editor/Hooks/MenuHooks.hpp"
#include "bs-android-editor/UI/EditorFlowCoordinator.hpp"
#include "bs-android-editor/main.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "bsml/shared/BSML-Lite/Creation/Buttons.hpp"

#include "GlobalNamespace/MainMenuViewController.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/Vector2.hpp"

#include <array>
#include <exception>

using namespace GlobalNamespace;

namespace {

// Solo/Party/Campaign/Multiplayer's actual on-screen positions, read at
// runtime, rather than a hardcoded guess at where they are - keeps this
// button correctly placed under that row even if its own layout shifts
// between game versions. Falls back to the old hardcoded spot if none of
// the four are resolvable yet for some reason.
UnityEngine::Vector2 ComputeMapEditorButtonPosition(MainMenuViewController* self) {
    constexpr float kGapBelowLowestButton = 7.0f;
    const UnityEngine::Vector2 kFallbackPosition(-14.0f, -32.0f);

    const std::array<UnityEngine::UI::Button*, 4> modeButtons{self->_soloButton, self->_partyButton,
                                                               self->_campaignButton, self->_multiplayerButton};

    bool foundAny = false;
    float lowestY = 0.0f;
    float sumX = 0.0f;
    int count = 0;
    for (auto* button : modeButtons) {
        if (button == nullptr) continue;
        UnityEngine::RectTransform* rect = button->transform.cast<UnityEngine::RectTransform>();
        if (rect == nullptr) continue;

        const auto pos = rect->get_anchoredPosition();
        if (!foundAny || pos.y < lowestY) lowestY = pos.y;
        sumX += pos.x;
        ++count;
        foundAny = true;
    }

    if (!foundAny) return kFallbackPosition;
    return {sumX / static_cast<float>(count), lowestY - kGapBelowLowestButton};
}

} // namespace

MAKE_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void,
                MainMenuViewController* self, bool firstActivation, bool addedToHierarchy,
                bool screenSystemEnabling) {
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if (!firstActivation) return;

    // This runs the moment the main menu first appears, with no user
    // interaction - which makes it the first (and, on a stock main menu,
    // only) place our own code runs unprompted every single time the game
    // is opened. A try/catch here can't do anything about a hard native
    // crash, but it does mean a thrown C++ exception (e.g. from a metadata
    // lookup failing) gets logged instead of silently taking the whole
    // game down with it, and the log line either side pins down whether we
    // got this far at all.
    Logger.info("MainMenuViewController_DidActivate: adding Map Editor button");
    try {
        BSML::Lite::CreateUIButton(self->get_transform(), "Map Editor", ComputeMapEditorButtonPosition(self),
                                    [] { bs_editor::ui::PresentEditorFlow(); });
        Logger.info("MainMenuViewController_DidActivate: Map Editor button added");
    } catch (const std::exception& e) {
        Logger.error("MainMenuViewController_DidActivate: failed to add Map Editor button: {}", e.what());
    } catch (...) {
        Logger.error("MainMenuViewController_DidActivate: failed to add Map Editor button: unknown exception");
    }
}

namespace bs_editor::hooks {

void InstallMenuHooks() {
    INSTALL_HOOK(Logger, MainMenuViewController_DidActivate);
}

} // namespace bs_editor::hooks
