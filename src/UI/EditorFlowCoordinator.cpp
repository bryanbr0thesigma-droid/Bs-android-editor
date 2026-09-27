#include "bs-android-editor/UI/EditorFlowCoordinator.hpp"
#include "bs-android-editor/UI/EditorViewController.hpp"
#include "bs-android-editor/main.hpp"

#include "bsml/shared/Helpers/creation.hpp"
#include "bsml/shared/Helpers/getters.hpp"

DEFINE_TYPE(BSAndroidEditor, EditorFlowCoordinator);

using namespace BSAndroidEditor;

void EditorFlowCoordinator::DidActivate(bool firstActivation, bool /*addedToHierarchy*/,
                                         bool /*screenSystemEnabling*/) {
    if (!firstActivation) return;

    SetTitle("Map Editor", HMUI::ViewController_AnimationType::In);
    showBackButton = true;

    ProvideInitialViewControllers(BSML::Helpers::CreateViewController<EditorViewController*>(), nullptr, nullptr,
                                   nullptr, nullptr);
}

void EditorFlowCoordinator::BackButtonWasPressed(HMUI::ViewController* /*topViewController*/) {
    _parentFlowCoordinator->DismissFlowCoordinator(this, HMUI::ViewController_AnimationDirection::Horizontal,
                                                    nullptr, false);
}

namespace bs_editor::ui {

void PresentEditorFlow() {
    static EditorFlowCoordinator* instance = nullptr;
    if (!instance) {
        instance = BSML::Helpers::CreateFlowCoordinator<EditorFlowCoordinator*>();
    }

    BSML::Helpers::GetMainFlowCoordinator()->PresentFlowCoordinator(
        instance, nullptr, HMUI::ViewController_AnimationDirection::Horizontal, false, false);
}

} // namespace bs_editor::ui
