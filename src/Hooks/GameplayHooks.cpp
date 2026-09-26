// Per-frame bridge between real VR controller input and EditorController,
// plus keeping the game's own audio playback in sync with the editor's
// playhead (play/pause/scrub).
//
// Verified against bs-cordl 4008.0.0 (Beat Saber 1.40.8_7379,
// GlobalNamespace::VRController, GlobalNamespace::AudioTimeSyncController)
// and UnityEngine::Time — identical API surface to the newer bs-cordl
// 4500.1.0 originally checked, so only the beatsaber-hook 6.4.2 include
// path below (shared/utils/hooking.hpp, not the flat shared/hooking.hpp
// used from beatsaber-hook ~7.x+) needed updating for this game version.
// Unlike
// an earlier draft of this file, this does NOT use UnityEngine::XR::
// InputDevice/CommonUsages: that codegen dump has no TryGetFeatureValue on
// InputDevice at all (likely stripped as an unused generic method), so
// controller pose/trigger/thumbstick come from Beat Saber's own
// GlobalNamespace::VRController component instead, which is what the game
// itself uses for saber tracking and exposes exactly what we need
// (position, thumbstick, triggerValue) as plain properties.
//
// Control mapping: right hand places/deletes/drags (mirrors the base
// game's saber hand for cutting notes); left hand's thumbstick X scrubs
// the playhead and its trigger toggles play/pause.
#include "bs-android-editor/Hooks/GameplayHooks.hpp"
#include "bs-android-editor/EditorSession.hpp"
#include "bs-android-editor/main.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/VRController.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/XR/XRNode.hpp"

#include <cmath>

using namespace GlobalNamespace;

namespace {

constexpr float kTriggerDownThreshold = 0.5f;
constexpr float kGridOriginHeightMeters = 1.0f;
constexpr double kResyncEpsilonSeconds = 1.0 / 1000.0;

VRController* g_leftController = nullptr;
VRController* g_rightController = nullptr;
bool g_previousLeftTriggerDown = false;
bool g_previousRightTriggerDown = false;

} // namespace

// VRController is a per-hand MonoBehaviour that already runs every frame;
// piggybacking on its Update() to cache each hand's instance avoids the
// extra custom-types boilerplate a brand-new MonoBehaviour would need (see
// the note in the previous version of this file), same rationale as
// hooking AudioTimeSyncController::Update below.
MAKE_HOOK_MATCH(VRController_Update, &VRController::Update, void, VRController* self) {
    VRController_Update(self);

    if (self->get_node() == UnityEngine::XR::XRNode::LeftHand) {
        g_leftController = self;
    } else if (self->get_node() == UnityEngine::XR::XRNode::RightHand) {
        g_rightController = self;
    }
}

MAKE_HOOK_MATCH(AudioTimeSyncController_Update, &AudioTimeSyncController::Update, void,
                AudioTimeSyncController* self) {
    auto& session = bs_editor::EditorSession::Instance();
    if (!session.IsActive()) {
        AudioTimeSyncController_Update(self);
        return;
    }

    // Editor mode owns time while active: we deliberately skip the
    // original Update (which would otherwise advance songTime from actual
    // playback) and drive everything from EditorController's own playhead.
    bs_editor::EditorController* controller = session.controller();
    const float deltaTime = UnityEngine::Time::get_deltaTime();

    float scrubAxis = 0.0f;
    if (g_leftController != nullptr) {
        const auto thumbstick = g_leftController->get_thumbstick();
        scrubAxis = thumbstick.x;

        const bool leftTriggerDown = g_leftController->get_triggerValue() > kTriggerDownThreshold;
        if (leftTriggerDown && !g_previousLeftTriggerDown) {
            controller->TogglePlaying();
            if (controller->isPlaying()) {
                self->Resume();
            } else {
                self->Pause();
            }
        }
        g_previousLeftTriggerDown = leftTriggerDown;
    }

    const double beforeSeconds = controller->currentTimeSeconds();
    controller->AdvanceTime(deltaTime, scrubAxis);
    const double afterSeconds = controller->currentTimeSeconds();
    if (std::fabs(afterSeconds - beforeSeconds) > kResyncEpsilonSeconds) {
        self->SeekTo(static_cast<float>(afterSeconds));
    }

    if (g_rightController != nullptr) {
        bs_editor::ControllerFrame frame;

        const auto position = g_rightController->get_position();
        frame.localPosition = {position.x, position.y - kGridOriginHeightMeters};

        const auto thumbstick = g_rightController->get_thumbstick();
        frame.aimMagnitude = std::sqrt(thumbstick.x * thumbstick.x + thumbstick.y * thumbstick.y);
        frame.aimAngleDegrees = std::atan2(thumbstick.x, thumbstick.y) * (180.0f / static_cast<float>(M_PI));

        const bool triggerDown = g_rightController->get_triggerValue() > kTriggerDownThreshold;
        frame.triggerDown = triggerDown;
        frame.triggerPressedThisFrame = triggerDown && !g_previousRightTriggerDown;
        frame.triggerReleasedThisFrame = !triggerDown && g_previousRightTriggerDown;
        g_previousRightTriggerDown = triggerDown;

        controller->ProcessFrame(frame);
    }
}

namespace bs_editor::hooks {

void InstallGameplayHooks() {
    INSTALL_HOOK(Logger, VRController_Update);
    INSTALL_HOOK(Logger, AudioTimeSyncController_Update);
}

} // namespace bs_editor::hooks
