#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

// Data model for the Beat Saber "v3" difficulty file format and the
// (legacy-style, still universally supported) Info.dat song manifest.
//
// Only the fields the in-headset editor actually creates/edits are modeled
// as structured C++ types. Everything else present in a loaded file (arcs,
// chains, light event box groups, custom `_customData`, etc.) is kept as
// raw JSON in `extras` on every struct and merged back in untouched on
// save, so loading a map made in ChroMapper and re-saving it from this
// editor never silently drops content this tool doesn't understand yet.

namespace bs_editor::core {

enum class NoteColor : int { Red = 0, Blue = 1 };

// Matches Beat Saber's cut direction enum values exactly.
enum class CutDirection : int {
    Up = 0,
    Down = 1,
    Left = 2,
    Right = 3,
    UpLeft = 4,
    UpRight = 5,
    DownLeft = 6,
    DownRight = 7,
    Any = 8, // "dot" note
};

struct BpmEvent {
    double b = 0.0; // beat
    double m = 120.0; // bpm from this beat onward
};

struct ColorNote {
    double b = 0.0; // beat
    int x = 1; // line index, 0-3 (left to right)
    int y = 0; // line layer, 0-2 (bottom to top)
    NoteColor c = NoteColor::Red;
    CutDirection d = CutDirection::Up;
    int a = 0; // extra angle offset in degrees, added on top of `d`
    nlohmann::json extras = nlohmann::json::object();
};

struct BombNote {
    double b = 0.0;
    int x = 1;
    int y = 0;
    nlohmann::json extras = nlohmann::json::object();
};

struct Obstacle {
    double b = 0.0; // start beat
    double duration = 1.0; // length in beats
    int x = 1;
    int y = 0;
    double width = 1.0;
    double height = 1.0;
    nlohmann::json extras = nlohmann::json::object();
};

// A "basic" lighting/event trigger (light on/off/flash, ring rotation
// trigger, etc). Full light-event-box-group choreography is out of scope
// for the in-headset editor and is preserved verbatim via `extras` on the
// containing BeatmapDifficulty instead.
struct BasicEvent {
    double b = 0.0;
    int et = 0; // event type
    int i = 0; // value
    double f = 1.0; // float value (brightness)
    nlohmann::json extras = nlohmann::json::object();
};

struct BeatmapDifficulty {
    std::string version = "3.3.0";
    std::vector<BpmEvent> bpmEvents;
    std::vector<ColorNote> colorNotes;
    std::vector<BombNote> bombNotes;
    std::vector<Obstacle> obstacles;
    std::vector<BasicEvent> basicBeatmapEvents;

    // Full original JSON object for this difficulty file (or an empty
    // object for a brand new one). Fields this editor understands are
    // overwritten from the structured members above on save; every other
    // field (sliders, burstSliders, waypoints, lightColorEventBoxGroups,
    // customData, ...) passes through unchanged.
    nlohmann::json extras = nlohmann::json::object();
};

struct DifficultyBeatmap {
    std::string difficulty = "Normal"; // Easy/Normal/Hard/Expert/ExpertPlus
    int difficultyRank = 3;
    std::string beatmapFilename;
    double noteJumpMovementSpeed = 16.0;
    double noteJumpStartBeatOffset = 0.0;
    nlohmann::json extras = nlohmann::json::object();
};

struct DifficultyBeatmapSet {
    std::string beatmapCharacteristicName = "Standard";
    std::vector<DifficultyBeatmap> difficultyBeatmaps;
    nlohmann::json extras = nlohmann::json::object();
};

struct SongInfo {
    std::string songName;
    std::string songSubName;
    std::string songAuthorName;
    std::string levelAuthorName;
    double beatsPerMinute = 120.0;
    double songTimeOffset = 0.0;
    double previewStartTime = 12.0;
    double previewDuration = 10.0;
    std::string songFilename = "song.egg";
    std::string coverImageFilename = "cover.jpg";
    std::string environmentName = "DefaultEnvironment";
    std::vector<DifficultyBeatmapSet> difficultyBeatmapSets;

    nlohmann::json extras = nlohmann::json::object();
};

} // namespace bs_editor::core
