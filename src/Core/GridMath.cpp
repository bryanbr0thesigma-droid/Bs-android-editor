#include "bs-android-editor/Core/GridMath.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace bs_editor::core {

namespace {
int ClampInt(int value, int lo, int hi) {
    return std::max(lo, std::min(hi, value));
}
} // namespace

Vec2 GridCellToLocalPosition(int lineIndex, int lineLayer) {
    // Center the 4 columns around x=0: columns are at indices 0..3, so the
    // midpoint sits between columns 1 and 2.
    const float centeredIndex = static_cast<float>(lineIndex) - (kGridColumns - 1) / 2.0f;
    Vec2 pos;
    pos.x = centeredIndex * kColumnSpacingMeters;
    pos.y = kGridBaseHeightMeters + static_cast<float>(lineLayer) * kRowSpacingMeters;
    return pos;
}

void LocalPositionToGridCell(Vec2 position, int& outLineIndex, int& outLineLayer) {
    const float centeredIndex = position.x / kColumnSpacingMeters;
    const int lineIndex = static_cast<int>(std::lround(centeredIndex + (kGridColumns - 1) / 2.0f));
    const int lineLayer = static_cast<int>(std::lround((position.y - kGridBaseHeightMeters) / kRowSpacingMeters));

    outLineIndex = ClampInt(lineIndex, 0, kGridColumns - 1);
    outLineLayer = ClampInt(lineLayer, 0, kGridRows - 1);
}

CutDirection AngleToCutDirection(float angleDegrees, float joystickMagnitude, float deadzoneRadius) {
    if (joystickMagnitude < deadzoneRadius) {
        return CutDirection::Any;
    }

    // Normalize into [0, 360).
    float angle = std::fmod(angleDegrees, 360.0f);
    if (angle < 0.0f) angle += 360.0f;

    // 8 compass directions, 45 degrees apart, matching Beat Saber's cut
    // direction enum ordering (0=up, clockwise).
    static constexpr std::array<CutDirection, 8> kOrder = {
        CutDirection::Up, CutDirection::UpRight, CutDirection::Right, CutDirection::DownRight,
        CutDirection::Down, CutDirection::DownLeft, CutDirection::Left, CutDirection::UpLeft,
    };

    const int index = static_cast<int>(std::lround(angle / 45.0f)) % 8;
    return kOrder[static_cast<size_t>(index)];
}

float CutDirectionToAngle(CutDirection direction) {
    switch (direction) {
        case CutDirection::Up: return 0.0f;
        case CutDirection::UpRight: return 45.0f;
        case CutDirection::Right: return 90.0f;
        case CutDirection::DownRight: return 135.0f;
        case CutDirection::Down: return 180.0f;
        case CutDirection::DownLeft: return 225.0f;
        case CutDirection::Left: return 270.0f;
        case CutDirection::UpLeft: return 315.0f;
        case CutDirection::Any: return 0.0f;
    }
    return 0.0f;
}

BeatTimeConverter::BeatTimeConverter(double baseBpm, std::vector<BpmEvent> bpmEvents) {
    std::sort(bpmEvents.begin(), bpmEvents.end(), [](const BpmEvent& a, const BpmEvent& b) {
        return a.b < b.b;
    });

    double seconds = 0.0;
    double beat = 0.0;
    double bpm = baseBpm > 0.0 ? baseBpm : 120.0;
    segments_.push_back({beat, seconds, bpm});

    for (const auto& event : bpmEvents) {
        if (event.b <= beat) {
            // Same-beat or out-of-order BPM change: just replace the
            // current segment's rate rather than emitting a zero-length one.
            segments_.back().bpm = event.m;
            bpm = event.m;
            continue;
        }
        const double elapsedBeats = event.b - beat;
        seconds += (elapsedBeats / bpm) * 60.0;
        beat = event.b;
        bpm = event.m > 0.0 ? event.m : bpm;
        segments_.push_back({beat, seconds, bpm});
    }
}

double BeatTimeConverter::BeatToSeconds(double beat) const {
    const Segment* active = &segments_.front();
    for (const auto& segment : segments_) {
        if (segment.startBeat > beat) break;
        active = &segment;
    }
    const double elapsedBeats = beat - active->startBeat;
    return active->startSeconds + (elapsedBeats / active->bpm) * 60.0;
}

double BeatTimeConverter::SecondsToBeat(double seconds) const {
    const Segment* active = &segments_.front();
    for (const auto& segment : segments_) {
        if (segment.startSeconds > seconds) break;
        active = &segment;
    }
    const double elapsedSeconds = seconds - active->startSeconds;
    return active->startBeat + (elapsedSeconds / 60.0) * active->bpm;
}

double BeatTimeConverter::SnapBeat(double beat, int subdivision) {
    if (subdivision <= 0) return beat;
    const double step = 1.0 / static_cast<double>(subdivision);
    return std::round(beat / step) * step;
}

} // namespace bs_editor::core
