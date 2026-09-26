#pragma once

#include <cstddef>

#include "bs-android-editor/Core/EditorDocument.hpp"
#include "bs-android-editor/Core/GridMath.hpp"
#include "bs-android-editor/Core/MapTypes.hpp"

// The bridge between "some controller gave us a frame of input" and "the
// document got edited". Deliberately takes a plain-data ControllerFrame
// instead of any VR/IL2CPP type, so:
//   - it is unit-testable on a desktop compiler (see tests/), and
//   - src/Hooks/GameplayHooks.cpp is the *only* file that has to translate
//     real Quest controller poses/buttons into this shape, keeping the
//     unverifiable, version-sensitive IL2CPP glue as small as possible.

namespace bs_editor {

enum class EditorTool { Note, Bomb, Obstacle, Event, Delete };

struct ControllerFrame {
    core::Vec2 localPosition; // controller tip projected onto the grid plane, meters, relative to grid center
    float aimAngleDegrees = 0.0f; // 0 = up, clockwise-positive (thumbstick angle or wrist roll)
    float aimMagnitude = 0.0f; // 0..1, how far off-center the aim is (drives dot-note deadzone)
    bool triggerDown = false;
    bool triggerPressedThisFrame = false;
    bool triggerReleasedThisFrame = false;
    double songTimeSeconds = 0.0;
};

class EditorController {
public:
    struct CursorPreview {
        int lineIndex = 0;
        int lineLayer = 0;
        double beat = 0.0;
        core::CutDirection direction = core::CutDirection::Up;
    };

    EditorController(core::EditorDocument& document, double baseBpm, int snapSubdivision = 8);

    void SetTool(EditorTool tool) { tool_ = tool; }
    EditorTool tool() const { return tool_; }

    void SetActiveColor(core::NoteColor color) { activeColor_ = color; }
    core::NoteColor activeColor() const { return activeColor_; }

    void SetActiveEventType(int eventType) { activeEventType_ = eventType; }
    void SetSnapSubdivision(int subdivision) { snapSubdivision_ = subdivision; }
    int snapSubdivision() const { return snapSubdivision_; }

    // Rebuilds the beat<->time conversion from the document's current
    // bpmEvents. Call after loading a document or editing bpmEvents.
    void RefreshBpmTimeline();

    CursorPreview PreviewAt(const ControllerFrame& frame) const;

    // Call once per frame (e.g. from a hooked Update()) with the latest
    // controller state. Dispatches placement/drag/delete based on the
    // active tool and trigger edge transitions in `frame`.
    void ProcessFrame(const ControllerFrame& frame);

    void Undo() { document_.Undo(); }
    void Redo() { document_.Redo(); }

    bool IsObstacleDragActive() const { return obstacleDragActive_; }

private:
    void PlaceAtCursor(const ControllerFrame& frame);
    void StartObstacleDrag(const ControllerFrame& frame);
    void FinishObstacleDrag(const ControllerFrame& frame);
    void DeleteNearestAtCursor(const ControllerFrame& frame);

    core::EditorDocument& document_;
    double baseBpm_;
    core::BeatTimeConverter timeConverter_;

    EditorTool tool_ = EditorTool::Note;
    core::NoteColor activeColor_ = core::NoteColor::Red;
    int activeEventType_ = 0;
    int snapSubdivision_;

    bool obstacleDragActive_ = false;
    double obstacleStartBeat_ = 0.0;
    int obstacleStartLineIndex_ = 0;
};

} // namespace bs_editor
