#pragma once

#include <optional>
#include <vector>

#include "bs-android-editor/Core/MapTypes.hpp"

// Pure math for turning real-world/VR-space quantities into beatmap grid
// quantities and back. None of this touches the game engine, so it is
// fully unit-testable on a desktop compiler; the Quest-side hook layer
// (src/Hooks) is the only place that has to feed it real controller data.

namespace bs_editor::core {

// Distance in meters between adjacent grid columns/rows in Beat Saber's
// standard note grid. These match the base game's default (non-Mapping
// Extensions) grid and are a reasonable approximation for cursor snapping;
// see README for how the Quest-side hook can pull the live values instead.
constexpr float kColumnSpacingMeters = 0.6f;
constexpr float kRowSpacingMeters = 0.55f;
constexpr float kGridBaseHeightMeters = 0.9f; // height of row 0 above the floor
constexpr int kGridColumns = 4; // valid x: 0-3
constexpr int kGridRows = 3; // valid y: 0-2

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

// Converts a grid cell to a position in meters relative to the player,
// centered horizontally on the 4-column grid.
Vec2 GridCellToLocalPosition(int lineIndex, int lineLayer);

// Inverse of GridCellToLocalPosition; result is clamped to valid grid
// bounds ([0, kGridColumns-1], [0, kGridRows-1]).
void LocalPositionToGridCell(Vec2 position, int& outLineIndex, int& outLineLayer);

// Snaps an arbitrary angle (degrees, 0 = up, clockwise-positive) to the
// nearest of Beat Saber's 8 cut directions. `deadzoneRadius` in [0,1] is
// the normalized joystick magnitude below which the note should be a "dot"
// (Any) note instead of a directional one, matching how players hold a
// note flat to swing any direction.
CutDirection AngleToCutDirection(float angleDegrees, float joystickMagnitude, float deadzoneRadius = 0.3f);

// Inverse of AngleToCutDirection (Any has no canonical angle; returns 0).
float CutDirectionToAngle(CutDirection direction);

// Converts between musical beats and wall-clock seconds given a sorted
// timeline of BPM changes (as stored in a v3 difficulty file) plus the
// song's base BPM from Info.dat. `bpmEvents` does not need to be
// pre-sorted; both directions handle an empty list by using `baseBpm`
// for the whole song.
class BeatTimeConverter {
public:
    explicit BeatTimeConverter(double baseBpm, std::vector<BpmEvent> bpmEvents = {});

    double BeatToSeconds(double beat) const;
    double SecondsToBeat(double seconds) const;

    // Rounds `beat` to the nearest multiple of 1/`subdivision` of a beat.
    // subdivision=4 snaps to 1/4 notes, 16 snaps to 1/16th, etc. Passing 0
    // disables snapping (returns `beat` unchanged).
    static double SnapBeat(double beat, int subdivision);

private:
    struct Segment {
        double startBeat;
        double startSeconds;
        double bpm;
    };
    std::vector<Segment> segments_;
};

} // namespace bs_editor::core
