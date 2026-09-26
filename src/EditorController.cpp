#include "bs-android-editor/EditorController.hpp"

#include <algorithm>
#include <cmath>

namespace bs_editor {

using core::AngleToCutDirection;
using core::BasicEvent;
using core::BeatTimeConverter;
using core::BombNote;
using core::ColorNote;
using core::LocalPositionToGridCell;
using core::Obstacle;
using core::kGridRows;

namespace {
constexpr double kDeleteBeatTolerance = 0.25;
}

EditorController::EditorController(core::EditorDocument& document, double baseBpm, int snapSubdivision)
    : document_(document),
      baseBpm_(baseBpm),
      timeConverter_(baseBpm, document.difficulty().bpmEvents),
      snapSubdivision_(snapSubdivision) {}

void EditorController::RefreshBpmTimeline() {
    timeConverter_ = BeatTimeConverter(baseBpm_, document_.difficulty().bpmEvents);
}

EditorController::CursorPreview EditorController::PreviewAt(const ControllerFrame& frame) const {
    CursorPreview preview;
    const double rawBeat = timeConverter_.SecondsToBeat(frame.songTimeSeconds);
    preview.beat = BeatTimeConverter::SnapBeat(rawBeat, snapSubdivision_);
    LocalPositionToGridCell(frame.localPosition, preview.lineIndex, preview.lineLayer);
    preview.direction = AngleToCutDirection(frame.aimAngleDegrees, frame.aimMagnitude);
    return preview;
}

void EditorController::ProcessFrame(const ControllerFrame& frame) {
    if (frame.triggerPressedThisFrame) {
        switch (tool_) {
            case EditorTool::Note:
            case EditorTool::Bomb:
            case EditorTool::Event:
                PlaceAtCursor(frame);
                break;
            case EditorTool::Obstacle:
                StartObstacleDrag(frame);
                break;
            case EditorTool::Delete:
                DeleteNearestAtCursor(frame);
                break;
        }
    }

    if (tool_ == EditorTool::Obstacle && obstacleDragActive_ && frame.triggerReleasedThisFrame) {
        FinishObstacleDrag(frame);
    }
}

void EditorController::PlaceAtCursor(const ControllerFrame& frame) {
    const CursorPreview preview = PreviewAt(frame);

    switch (tool_) {
        case EditorTool::Note: {
            ColorNote note;
            note.b = preview.beat;
            note.x = preview.lineIndex;
            note.y = preview.lineLayer;
            note.c = activeColor_;
            note.d = preview.direction;
            document_.AddColorNote(note);
            break;
        }
        case EditorTool::Bomb: {
            BombNote bomb;
            bomb.b = preview.beat;
            bomb.x = preview.lineIndex;
            bomb.y = preview.lineLayer;
            document_.AddBombNote(bomb);
            break;
        }
        case EditorTool::Event: {
            BasicEvent event;
            event.b = preview.beat;
            event.et = activeEventType_;
            event.i = 1;
            event.f = 1.0;
            document_.AddBasicEvent(event);
            break;
        }
        default:
            break;
    }
}

void EditorController::StartObstacleDrag(const ControllerFrame& frame) {
    const CursorPreview preview = PreviewAt(frame);
    obstacleStartBeat_ = preview.beat;
    obstacleStartLineIndex_ = preview.lineIndex;
    obstacleDragActive_ = true;
}

void EditorController::FinishObstacleDrag(const ControllerFrame& frame) {
    const CursorPreview preview = PreviewAt(frame);
    obstacleDragActive_ = false;

    const double minSpan = 1.0 / std::max(snapSubdivision_, 1);
    const double startBeat = std::min(obstacleStartBeat_, preview.beat);
    const double endBeat = std::max(obstacleStartBeat_, preview.beat);

    Obstacle obstacle;
    obstacle.b = startBeat;
    obstacle.duration = std::max(endBeat - startBeat, minSpan);
    obstacle.x = std::min(obstacleStartLineIndex_, preview.lineIndex);
    obstacle.width = std::abs(preview.lineIndex - obstacleStartLineIndex_) + 1;
    obstacle.y = 0;
    obstacle.height = kGridRows; // MVP: obstacles are always full-height walls
    document_.AddObstacle(obstacle);
}

void EditorController::DeleteNearestAtCursor(const ControllerFrame& frame) {
    const CursorPreview preview = PreviewAt(frame);
    const auto& diff = document_.difficulty();

    enum class Kind { None, Note, Bomb, Obstacle, Event };
    Kind bestKind = Kind::None;
    std::size_t bestIndex = 0;
    double bestDist = kDeleteBeatTolerance;

    for (std::size_t i = 0; i < diff.colorNotes.size(); ++i) {
        const auto& note = diff.colorNotes[i];
        if (note.x != preview.lineIndex || note.y != preview.lineLayer) continue;
        const double dist = std::abs(note.b - preview.beat);
        if (dist < bestDist) {
            bestDist = dist;
            bestKind = Kind::Note;
            bestIndex = i;
        }
    }
    for (std::size_t i = 0; i < diff.bombNotes.size(); ++i) {
        const auto& bomb = diff.bombNotes[i];
        if (bomb.x != preview.lineIndex || bomb.y != preview.lineLayer) continue;
        const double dist = std::abs(bomb.b - preview.beat);
        if (dist < bestDist) {
            bestDist = dist;
            bestKind = Kind::Bomb;
            bestIndex = i;
        }
    }
    for (std::size_t i = 0; i < diff.obstacles.size(); ++i) {
        const auto& obstacle = diff.obstacles[i];
        if (preview.lineIndex < obstacle.x || preview.lineIndex >= obstacle.x + obstacle.width) continue;
        if (preview.beat < obstacle.b || preview.beat > obstacle.b + obstacle.duration) continue;
        const double dist = std::abs(obstacle.b - preview.beat);
        if (dist < bestDist) {
            bestDist = dist;
            bestKind = Kind::Obstacle;
            bestIndex = i;
        }
    }
    for (std::size_t i = 0; i < diff.basicBeatmapEvents.size(); ++i) {
        const auto& event = diff.basicBeatmapEvents[i];
        const double dist = std::abs(event.b - preview.beat);
        if (dist < bestDist) {
            bestDist = dist;
            bestKind = Kind::Event;
            bestIndex = i;
        }
    }

    switch (bestKind) {
        case Kind::Note: document_.RemoveColorNoteAt(bestIndex); break;
        case Kind::Bomb: document_.RemoveBombNoteAt(bestIndex); break;
        case Kind::Obstacle: document_.RemoveObstacleAt(bestIndex); break;
        case Kind::Event: document_.RemoveBasicEventAt(bestIndex); break;
        case Kind::None: break;
    }
}

} // namespace bs_editor
