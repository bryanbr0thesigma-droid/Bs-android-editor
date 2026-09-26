// Per-frame bridge between real VR controller input and EditorController.
//
// Design choice: this piggybacks on the game's *existing* per-frame
// AudioTimeSyncController::Update hook rather than injecting a brand-new
// custom MonoBehaviour component. Registering a new IL2CPP-visible
// MonoBehaviour (so Unity's message system will call its Update/Start) needs
// extra boilerplate from the `custom-types` package whose exact macro
// spelling drifts across beatsaber-hook/cordl releases; reusing an existing,
// already-hookable Update method sidesteps that and keeps this file's
// version-sensitive surface as small as possible.
//
// VERSION-SENSITIVE FILE: verify against your own extern/includes:
//   - GlobalNamespace::AudioTimeSyncController and its Update()/songTime
//     have changed shape across Beat Saber updates more than most classes.
//   - UnityEngine::XR::InputDevices / CommonUsages are stock Unity XR API
//     (not Beat Saber's own code), so they're far more stable across game
//     versions, but the exact out-parameter idiom (ByRef<T> vs T&) depends
//     on your bs-cordl version's codegen style.
#include "bs-android-editor/Hooks/GameplayHooks.hpp"
#include "bs-android-editor/EditorSession.hpp"
#include "bs-android-editor/main.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "UnityEngine/XR/CommonUsages.hpp"
#include "UnityEngine/XR/InputDevice.hpp"
#include "UnityEngine/XR/InputDevices.hpp"
#include "UnityEngine/XR/XRNode.hpp"

#include <cmath>

using namespace GlobalNamespace;
using namespace UnityEngine::XR;

namespace {

bool g_previousTriggerDown = false;

// Tracking-space height (meters) of Beat Saber's grid center; used to
// re-center the raw controller position onto the editor's local grid
// coordinates. Approximate — replace with the real play-space/grid
// transform if you wire one up.
constexpr float kGridOriginHeightMeters = 1.0f;

bs_editor::ControllerFrame ReadRightControllerFrame(double songTimeSeconds) {
    bs_editor::ControllerFrame frame;
    frame.songTimeSeconds = songTimeSeconds;

    InputDevice rightHand = InputDevices::GetDeviceAtXRNode(XRNode::RightHand);

    UnityEngine::Vector3 position{};
    if (rightHand.TryGetFeatureValue(CommonUsages::get_devicePosition(), position)) {
        frame.localPosition = {position.x, position.y - kGridOriginHeightMeters};
    }

    UnityEngine::Vector2 thumbstick{};
    if (rightHand.TryGetFeatureValue(CommonUsages::get_primary2DAxis(), thumbstick)) {
        frame.aimMagnitude = std::sqrt(thumbstick.x * thumbstick.x + thumbstick.y * thumbstick.y);
        frame.aimAngleDegrees = std::atan2(thumbstick.x, thumbstick.y) * (180.0f / static_cast<float>(M_PI));
    }

    bool triggerDown = false;
    rightHand.TryGetFeatureValue(CommonUsages::get_triggerButton(), triggerDown);
    frame.triggerDown = triggerDown;
    frame.triggerPressedThisFrame = triggerDown && !g_previousTriggerDown;
    frame.triggerReleasedThisFrame = !triggerDown && g_previousTriggerDown;
    g_previousTriggerDown = triggerDown;

    return frame;
}

} // namespace

MAKE_HOOK_MATCH(AudioTimeSyncController_Update, &AudioTimeSyncController::Update, void,
                AudioTimeSyncController* self) {
    auto& session = bs_editor::EditorSession::Instance();
    if (!session.IsActive()) {
        AudioTimeSyncController_Update(self);
        return;
    }

    // Editor mode owns time while active: the song clock only moves via the
    // editor's own scrubbing, so we deliberately skip the original Update
    // (which would otherwise keep advancing songTime during playback).
    const double songTime = self->get_songTime();
    session.controller()->ProcessFrame(ReadRightControllerFrame(songTime));
}

namespace bs_editor::hooks {

void InstallGameplayHooks() {
    INSTALL_HOOK(getLogger(), AudioTimeSyncController_Update);
}

} // namespace bs_editor::hooks
