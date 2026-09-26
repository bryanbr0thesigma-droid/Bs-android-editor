#include "bs-android-editor/UI/EditorFlowCoordinator.hpp"
#include "bs-android-editor/UI/EditorViewController.hpp"
#include "bs-android-editor/main.hpp"

#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/MainMenu/BSMLFlowCoordinator.hpp"

using namespace BSAndroidEditor;

void EditorFlowCoordinator::DidActivate(bool firstActivation, bool /*addedToHierarchy*/,
                                         bool /*screenSystemEnabling*/) {
    if (!firstActivation) return;

    SetTitle("Map Editor", HMUI::ViewController::AnimationType::In);
    showBackButton = true;

    ProvideInitialViewControllers(BSML::Lite::CreateViewController<EditorViewController*>(), nullptr,
                                   nullptr, nullptr, nullptr);
}

namespace bs_editor::ui {

void PresentEditorFlow() {
    static EditorFlowCoordinator* instance = nullptr;
    if (!instance) {
        instance = BSML::Lite::CreateFlowCoordinator<EditorFlowCoordinator*>();
    }

    BSML::Helpers::GetMainFlowCoordinator()->PresentFlowCoordinator(instance, nullptr,
                                                                      HMUI::ViewController::AnimationDirection::Horizontal,
                                                                      false, false);
}

} // namespace bs_editor::ui
