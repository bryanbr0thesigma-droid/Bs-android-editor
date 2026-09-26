// Injects a "Map Editor" button into the main menu.
//
// VERSION-SENSITIVE FILE: the exact type/method names below
// (GlobalNamespace::MainMenuViewController, its DidActivate signature) come
// from bs-cordl codegen generated against *your* copy of Beat Saber and can
// shift across game updates. Check them against extern/includes before
// building; this mirrors the standard "hook DidActivate, add a BSML button"
// pattern used by many existing Quest mods (e.g. how PlaylistManager and
// SongCore add their own main-menu buttons).
#include "bs-android-editor/Hooks/MenuHooks.hpp"
#include "bs-android-editor/UI/EditorFlowCoordinator.hpp"
#include "bs-android-editor/main.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "bsml/shared/BSML-Lite/Creation/Standard.hpp"

#include "GlobalNamespace/MainMenuViewController.hpp"
#include "UnityEngine/Vector2.hpp"

using namespace GlobalNamespace;

MAKE_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void,
                MainMenuViewController* self, bool firstActivation, bool addedToHierarchy,
                bool screenSystemEnabling) {
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if (!firstActivation) return;

    BSML::Lite::CreateUIButton(self->get_transform(), "Map Editor", UnityEngine::Vector2(-14.0f, -32.0f),
                                [] { bs_editor::ui::PresentEditorFlow(); });
}

namespace bs_editor::hooks {

void InstallMenuHooks() {
    INSTALL_HOOK(getLogger(), MainMenuViewController_DidActivate);
}

} // namespace bs_editor::hooks
