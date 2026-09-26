// Self-contained sanity tests for the engine-agnostic core editor library.
// No test framework dependency: build and run via tests/CMakeLists.txt.
// Exit code is nonzero if any check fails.

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "bs-android-editor/Core/BeatmapSerializer.hpp"
#include "bs-android-editor/Core/EditorDocument.hpp"
#include "bs-android-editor/Core/GridMath.hpp"
#include "bs-android-editor/Core/MapTypes.hpp"
#include "bs-android-editor/EditorController.hpp"
#include "bs-android-editor/EditorSession.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool condition, const char* description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::fprintf(stderr, "FAIL: %s\n", description);
    }
}

void CheckNear(double a, double b, double epsilon, const char* description) {
    Check(std::fabs(a - b) <= epsilon, description);
}

} // namespace

using namespace bs_editor;
using namespace bs_editor::core;

void TestGridRoundTrip() {
    for (int x = 0; x < kGridColumns; ++x) {
        for (int y = 0; y < kGridRows; ++y) {
            const Vec2 pos = GridCellToLocalPosition(x, y);
            int outX = -1;
            int outY = -1;
            LocalPositionToGridCell(pos, outX, outY);
            Check(outX == x && outY == y, "grid cell survives position round trip");
        }
    }
}

void TestCutDirections() {
    Check(AngleToCutDirection(0.0f, 1.0f) == CutDirection::Up, "angle 0 -> Up");
    Check(AngleToCutDirection(45.0f, 1.0f) == CutDirection::UpRight, "angle 45 -> UpRight");
    Check(AngleToCutDirection(90.0f, 1.0f) == CutDirection::Right, "angle 90 -> Right");
    Check(AngleToCutDirection(180.0f, 1.0f) == CutDirection::Down, "angle 180 -> Down");
    Check(AngleToCutDirection(270.0f, 1.0f) == CutDirection::Left, "angle 270 -> Left");
    Check(AngleToCutDirection(10.0f, 0.05f) == CutDirection::Any, "small joystick deflection -> dot note");
}

void TestBeatTimeConversion() {
    BeatTimeConverter constantBpm(120.0);
    CheckNear(constantBpm.BeatToSeconds(4.0), 2.0, 1e-9, "4 beats @ 120bpm = 2s");
    CheckNear(constantBpm.SecondsToBeat(2.0), 4.0, 1e-9, "2s @ 120bpm = 4 beats");

    const std::vector<BpmEvent> changes = {{8.0, 180.0}};
    BeatTimeConverter varying(120.0, changes);
    const double secondsAt12 = varying.BeatToSeconds(12.0);
    CheckNear(secondsAt12, 4.0 + (4.0 / 180.0) * 60.0, 1e-9, "beat 12 lands correctly after a bpm change");
    CheckNear(varying.SecondsToBeat(secondsAt12), 12.0, 1e-6, "seconds->beat inverts beat->seconds across a bpm change");

    CheckNear(BeatTimeConverter::SnapBeat(1.05, 4), 1.0, 1e-9, "snaps down to nearest 1/4 beat");
    CheckNear(BeatTimeConverter::SnapBeat(1.20, 4), 1.25, 1e-9, "snaps up to nearest 1/4 beat");
}

void TestSerializerRoundTrip() {
    const std::string input = R"({
        "version": "3.3.0",
        "bpmEvents": [{"b": 0, "m": 128}],
        "colorNotes": [{"b": 2, "x": 1, "y": 0, "c": 0, "d": 1, "a": 0}],
        "bombNotes": [{"b": 4, "x": 2, "y": 1}],
        "obstacles": [{"b": 1, "d": 2, "x": 0, "y": 0, "w": 1, "h": 3}],
        "basicBeatmapEvents": [{"b": 0, "et": 1, "i": 3, "f": 1.0}],
        "colorBoostBeatmapEvents": [{"b": 1, "o": true}],
        "customData": {"foo": "bar"}
    })";

    BeatmapDifficulty diff = ParseDifficulty(input);
    Check(diff.colorNotes.size() == 1, "parsed one color note");
    Check(diff.bombNotes.size() == 1, "parsed one bomb note");
    Check(diff.obstacles.size() == 1, "parsed one obstacle");
    Check(diff.basicBeatmapEvents.size() == 1, "parsed one basic event");
    Check(diff.extras.contains("colorBoostBeatmapEvents"), "unmodeled array preserved in extras");
    Check(diff.extras.contains("customData"), "customData preserved in extras");
    Check(!diff.extras.contains("colorNotes"), "modeled field is not duplicated into extras");

    BeatmapDifficulty reparsed = ParseDifficulty(SerializeDifficulty(diff));
    Check(reparsed.colorNotes.size() == 1, "round trip keeps the color note");
    Check(reparsed.extras.contains("colorBoostBeatmapEvents"), "round trip keeps the unmodeled field");
    Check(reparsed.extras["customData"]["foo"] == "bar", "round trip keeps customData's value");
    Check(reparsed.colorNotes[0].x == 1 && reparsed.colorNotes[0].y == 0, "round trip keeps note grid position");
}

void TestSongInfoRoundTrip() {
    const std::string input = R"({
        "_songName": "Test Song",
        "_beatsPerMinute": 128,
        "_difficultyBeatmapSets": [
            {"_beatmapCharacteristicName": "Standard", "_difficultyBeatmaps": [
                {"_difficulty": "Hard", "_difficultyRank": 7, "_beatmapFilename": "Hard.dat"}
            ]}
        ],
        "_customData": {"note": "keep me"}
    })";

    SongInfo info = ParseSongInfo(input);
    Check(info.songName == "Test Song", "parsed song name");
    Check(info.difficultyBeatmapSets.size() == 1, "parsed one difficulty beatmap set");
    Check(info.difficultyBeatmapSets[0].difficultyBeatmaps.size() == 1, "parsed one difficulty beatmap");
    Check(info.extras.contains("_customData"), "custom data preserved on parse");

    SongInfo reparsed = ParseSongInfo(SerializeSongInfo(info));
    Check(reparsed.difficultyBeatmapSets[0].difficultyBeatmaps[0].difficulty == "Hard",
          "round trip keeps difficulty name");
    Check(reparsed.extras["_customData"]["note"] == "keep me", "round trip keeps Info.dat customData");
}

void TestEditorDocumentUndoRedo() {
    EditorDocument doc;

    ColorNote note;
    note.b = 4.0;
    note.x = 2;
    note.y = 1;
    doc.AddColorNote(note);
    Check(doc.difficulty().colorNotes.size() == 1, "note added");

    doc.Undo();
    Check(doc.difficulty().colorNotes.empty(), "undo removes the added note");

    doc.Redo();
    Check(doc.difficulty().colorNotes.size() == 1, "redo re-adds the note");

    doc.MoveColorNote(0, 8.0, 3, 2);
    Check(doc.difficulty().colorNotes[0].b == 8.0, "move applied");
    doc.Undo();
    Check(doc.difficulty().colorNotes[0].b == 4.0 && doc.difficulty().colorNotes[0].x == 2,
          "undo of move restores original position");

    doc.RemoveColorNoteAt(0);
    Check(doc.difficulty().colorNotes.empty(), "remove applied");
    doc.Undo();
    Check(doc.difficulty().colorNotes.size() == 1 && doc.difficulty().colorNotes[0].x == 2,
          "undo of remove restores the note");
    Check(doc.CanRedo(), "redo is available to re-apply the undone remove");

    BombNote bomb;
    bomb.b = 1.0;
    doc.AddBombNote(bomb);
    Check(!doc.CanRedo(), "a fresh edit clears the redo stack");
}

void TestEditorController() {
    EditorDocument doc;
    EditorController controller(doc, /*baseBpm=*/120.0, /*snapSubdivision=*/4);
    controller.SetTool(EditorTool::Note);
    controller.SetActiveColor(NoteColor::Red);

    controller.SetCurrentTimeSeconds(2.0); // 4 beats @ 120bpm
    ControllerFrame placeFrame;
    placeFrame.localPosition = GridCellToLocalPosition(2, 1);
    placeFrame.aimAngleDegrees = 0.0f;
    placeFrame.aimMagnitude = 1.0f;
    placeFrame.triggerPressedThisFrame = true;
    placeFrame.triggerDown = true;
    controller.ProcessFrame(placeFrame);

    Check(doc.difficulty().colorNotes.size() == 1, "controller placed a note on trigger press");
    if (!doc.difficulty().colorNotes.empty()) {
        const auto& placed = doc.difficulty().colorNotes[0];
        Check(placed.x == 2 && placed.y == 1, "note placed at the cursor's grid cell");
        CheckNear(placed.b, 4.0, 1e-6, "note placed at the correctly snapped beat");
        Check(placed.d == CutDirection::Up, "note cut direction derived from aim angle");
    }

    controller.SetTool(EditorTool::Obstacle);
    controller.SetCurrentTimeSeconds(1.0);
    ControllerFrame dragStart;
    dragStart.localPosition = GridCellToLocalPosition(0, 0);
    dragStart.triggerPressedThisFrame = true;
    controller.ProcessFrame(dragStart);
    Check(controller.IsObstacleDragActive(), "obstacle drag starts on trigger press");

    controller.SetCurrentTimeSeconds(2.0);
    ControllerFrame dragEnd;
    dragEnd.localPosition = GridCellToLocalPosition(2, 0);
    dragEnd.triggerReleasedThisFrame = true;
    controller.ProcessFrame(dragEnd);
    Check(!controller.IsObstacleDragActive(), "obstacle drag ends on trigger release");
    Check(doc.difficulty().obstacles.size() == 1, "obstacle created from the drag");
    if (!doc.difficulty().obstacles.empty()) {
        const auto& obstacle = doc.difficulty().obstacles[0];
        Check(obstacle.x == 0 && obstacle.width == 3, "obstacle spans the dragged columns");
        Check(obstacle.duration == 2.0, "obstacle spans the dragged beats");
    }

    controller.SetTool(EditorTool::Delete);
    controller.SetCurrentTimeSeconds(2.0);
    ControllerFrame deleteFrame;
    deleteFrame.localPosition = GridCellToLocalPosition(2, 1);
    deleteFrame.triggerPressedThisFrame = true;
    controller.ProcessFrame(deleteFrame);
    Check(doc.difficulty().colorNotes.empty(), "delete tool removed the nearest note");
}

void TestPlaybackAndScrubbing() {
    EditorDocument doc;
    EditorController controller(doc, /*baseBpm=*/120.0, /*snapSubdivision=*/4);

    Check(!controller.isPlaying(), "playback starts paused");
    CheckNear(controller.currentTimeSeconds(), 0.0, 1e-9, "playhead starts at zero");

    // Paused: advancing time with no scrub input should not move the playhead.
    controller.AdvanceTime(/*deltaSeconds=*/1.0, /*scrubAxis=*/0.0f);
    CheckNear(controller.currentTimeSeconds(), 0.0, 1e-9, "paused playback does not advance the playhead");

    controller.SetPlaying(true);
    Check(controller.isPlaying(), "SetPlaying(true) starts playback");
    controller.AdvanceTime(1.0, 0.0f);
    CheckNear(controller.currentTimeSeconds(), 1.0, 1e-9, "playing advances the playhead by real time");

    controller.TogglePlaying();
    Check(!controller.isPlaying(), "TogglePlaying pauses");
    controller.AdvanceTime(1.0, 0.0f);
    CheckNear(controller.currentTimeSeconds(), 1.0, 1e-9, "pausing again stops the playhead");

    // Scrubbing works independent of play state, and small deflections
    // within the deadzone are ignored.
    controller.AdvanceTime(1.0, 0.05f);
    CheckNear(controller.currentTimeSeconds(), 1.0, 1e-9, "small scrub deflection is deadzoned");

    controller.AdvanceTime(1.0, 1.0f);
    Check(controller.currentTimeSeconds() > 1.0, "full scrub deflection seeks forward");

    controller.SetCurrentTimeSeconds(-5.0);
    CheckNear(controller.currentTimeSeconds(), 0.0, 1e-9, "playhead cannot be set negative");
}

void TestEditorSession() {
    EditorSession& session = EditorSession::Instance();
    Check(!session.IsActive(), "session starts inactive");

    SongInfo info;
    info.songName = "Session Test";
    BeatmapDifficulty difficulty;
    session.Start(info, difficulty, /*baseBpm=*/128.0);

    Check(session.IsActive(), "session active after Start");
    Check(session.document() != nullptr && session.controller() != nullptr,
          "Start creates a document and controller");
    Check(session.document()->info().songName == "Session Test", "session document keeps the song info");

    session.End();
    Check(!session.IsActive(), "session inactive after End");
    Check(session.document() == nullptr && session.controller() == nullptr, "End releases document and controller");
}

int main() {
    TestGridRoundTrip();
    TestCutDirections();
    TestBeatTimeConversion();
    TestSerializerRoundTrip();
    TestSongInfoRoundTrip();
    TestEditorDocumentUndoRedo();
    TestEditorController();
    TestPlaybackAndScrubbing();
    TestEditorSession();

    std::printf("%d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
