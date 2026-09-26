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
    INSTALL_HOOK(Logger, MainMenuViewController_DidActivate);
}

} // namespace bs_editor::hooks
