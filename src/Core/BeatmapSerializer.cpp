#include "bs-android-editor/Core/BeatmapSerializer.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>

#include "nlohmann/json.hpp"

namespace bs_editor::core {

using nlohmann::json;

namespace {

std::string ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw BeatmapIOError("Could not open file for reading: " + path);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void WriteFile(const std::string& path, const std::string& contents) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw BeatmapIOError("Could not open file for writing: " + path);
    }
    file << contents;
}

// Removes `keys` from `obj` in place; used so extras doesn't duplicate
// fields that are represented as structured members.
void StripKeys(json& obj, std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        obj.erase(key);
    }
}

json NoteToJson(const ColorNote& note) {
    json j = note.extras;
    j["b"] = note.b;
    j["x"] = note.x;
    j["y"] = note.y;
    j["c"] = static_cast<int>(note.c);
    j["d"] = static_cast<int>(note.d);
    j["a"] = note.a;
    return j;
}

ColorNote NoteFromJson(const json& j) {
    ColorNote note;
    note.b = j.value("b", 0.0);
    note.x = j.value("x", 1);
    note.y = j.value("y", 0);
    note.c = static_cast<NoteColor>(j.value("c", 0));
    note.d = static_cast<CutDirection>(j.value("d", 0));
    note.a = j.value("a", 0);
    note.extras = j;
    StripKeys(note.extras, {"b", "x", "y", "c", "d", "a"});
    return note;
}

json BombToJson(const BombNote& bomb) {
    json j = bomb.extras;
    j["b"] = bomb.b;
    j["x"] = bomb.x;
    j["y"] = bomb.y;
    return j;
}

BombNote BombFromJson(const json& j) {
    BombNote bomb;
    bomb.b = j.value("b", 0.0);
    bomb.x = j.value("x", 1);
    bomb.y = j.value("y", 0);
    bomb.extras = j;
    StripKeys(bomb.extras, {"b", "x", "y"});
    return bomb;
}

json ObstacleToJson(const Obstacle& obstacle) {
    json j = obstacle.extras;
    j["b"] = obstacle.b;
    j["d"] = obstacle.duration;
    j["x"] = obstacle.x;
    j["y"] = obstacle.y;
    j["w"] = obstacle.width;
    j["h"] = obstacle.height;
    return j;
}

Obstacle ObstacleFromJson(const json& j) {
    Obstacle obstacle;
    obstacle.b = j.value("b", 0.0);
    obstacle.duration = j.value("d", 1.0);
    obstacle.x = j.value("x", 1);
    obstacle.y = j.value("y", 0);
    obstacle.width = j.value("w", 1.0);
    obstacle.height = j.value("h", 1.0);
    obstacle.extras = j;
    StripKeys(obstacle.extras, {"b", "d", "x", "y", "w", "h"});
    return obstacle;
}

json BpmEventToJson(const BpmEvent& event) {
    return json{{"b", event.b}, {"m", event.m}};
}

BpmEvent BpmEventFromJson(const json& j) {
    BpmEvent event;
    event.b = j.value("b", 0.0);
    event.m = j.value("m", 120.0);
    return event;
}

json BasicEventToJson(const BasicEvent& event) {
    json j = event.extras;
    j["b"] = event.b;
    j["et"] = event.et;
    j["i"] = event.i;
    j["f"] = event.f;
    return j;
}

BasicEvent BasicEventFromJson(const json& j) {
    BasicEvent event;
    event.b = j.value("b", 0.0);
    event.et = j.value("et", 0);
    event.i = j.value("i", 0);
    event.f = j.value("f", 1.0);
    event.extras = j;
    StripKeys(event.extras, {"b", "et", "i", "f"});
    return event;
}

template <typename T, typename ToJsonFn>
json ArrayToJson(const std::vector<T>& items, ToJsonFn toJson) {
    json arr = json::array();
    for (const auto& item : items) {
        arr.push_back(toJson(item));
    }
    return arr;
}

template <typename T, typename FromJsonFn>
std::vector<T> ArrayFromJson(const json& parent, const char* key, FromJsonFn fromJson) {
    std::vector<T> items;
    if (!parent.contains(key) || !parent[key].is_array()) return items;
    for (const auto& element : parent[key]) {
        items.push_back(fromJson(element));
    }
    return items;
}

json DifficultyBeatmapToJson(const DifficultyBeatmap& beatmap) {
    json j = beatmap.extras;
    j["_difficulty"] = beatmap.difficulty;
    j["_difficultyRank"] = beatmap.difficultyRank;
    j["_beatmapFilename"] = beatmap.beatmapFilename;
    j["_noteJumpMovementSpeed"] = beatmap.noteJumpMovementSpeed;
    j["_noteJumpStartBeatOffset"] = beatmap.noteJumpStartBeatOffset;
    return j;
}

DifficultyBeatmap DifficultyBeatmapFromJson(const json& j) {
    DifficultyBeatmap beatmap;
    beatmap.difficulty = j.value("_difficulty", "Normal");
    beatmap.difficultyRank = j.value("_difficultyRank", 3);
    beatmap.beatmapFilename = j.value("_beatmapFilename", "");
    beatmap.noteJumpMovementSpeed = j.value("_noteJumpMovementSpeed", 16.0);
    beatmap.noteJumpStartBeatOffset = j.value("_noteJumpStartBeatOffset", 0.0);
    beatmap.extras = j;
    StripKeys(beatmap.extras, {"_difficulty", "_difficultyRank", "_beatmapFilename",
                               "_noteJumpMovementSpeed", "_noteJumpStartBeatOffset"});
    return beatmap;
}

json DifficultyBeatmapSetToJson(const DifficultyBeatmapSet& set) {
    json j = set.extras;
    j["_beatmapCharacteristicName"] = set.beatmapCharacteristicName;
    j["_difficultyBeatmaps"] = ArrayToJson(set.difficultyBeatmaps, DifficultyBeatmapToJson);
    return j;
}

DifficultyBeatmapSet DifficultyBeatmapSetFromJson(const json& j) {
    DifficultyBeatmapSet set;
    set.beatmapCharacteristicName = j.value("_beatmapCharacteristicName", "Standard");
    set.difficultyBeatmaps =
        ArrayFromJson<DifficultyBeatmap>(j, "_difficultyBeatmaps", DifficultyBeatmapFromJson);
    set.extras = j;
    StripKeys(set.extras, {"_beatmapCharacteristicName", "_difficultyBeatmaps"});
    return set;
}

} // namespace

BeatmapDifficulty ParseDifficulty(const std::string& jsonText) {
    json root = json::parse(jsonText, /*cb=*/nullptr, /*allow_exceptions=*/true);

    BeatmapDifficulty difficulty;
    difficulty.version = root.value("version", std::string("3.3.0"));
    difficulty.bpmEvents = ArrayFromJson<BpmEvent>(root, "bpmEvents", BpmEventFromJson);
    difficulty.colorNotes = ArrayFromJson<ColorNote>(root, "colorNotes", NoteFromJson);
    difficulty.bombNotes = ArrayFromJson<BombNote>(root, "bombNotes", BombFromJson);
    difficulty.obstacles = ArrayFromJson<Obstacle>(root, "obstacles", ObstacleFromJson);
    difficulty.basicBeatmapEvents =
        ArrayFromJson<BasicEvent>(root, "basicBeatmapEvents", BasicEventFromJson);

    difficulty.extras = root;
    StripKeys(difficulty.extras,
              {"version", "bpmEvents", "colorNotes", "bombNotes", "obstacles", "basicBeatmapEvents"});
    return difficulty;
}

std::string SerializeDifficulty(const BeatmapDifficulty& difficulty, int indent) {
    json root = difficulty.extras;
    root["version"] = difficulty.version;
    root["bpmEvents"] = ArrayToJson(difficulty.bpmEvents, BpmEventToJson);
    root["colorNotes"] = ArrayToJson(difficulty.colorNotes, NoteToJson);
    root["bombNotes"] = ArrayToJson(difficulty.bombNotes, BombToJson);
    root["obstacles"] = ArrayToJson(difficulty.obstacles, ObstacleToJson);
    root["basicBeatmapEvents"] = ArrayToJson(difficulty.basicBeatmapEvents, BasicEventToJson);
    return root.dump(indent);
}

BeatmapDifficulty LoadDifficultyFile(const std::string& path) {
    try {
        return ParseDifficulty(ReadFile(path));
    } catch (const json::exception& e) {
        throw BeatmapIOError("Failed to parse difficulty file '" + path + "': " + e.what());
    }
}

void SaveDifficultyFile(const std::string& path, const BeatmapDifficulty& difficulty) {
    WriteFile(path, SerializeDifficulty(difficulty));
}

SongInfo ParseSongInfo(const std::string& jsonText) {
    json root = json::parse(jsonText, /*cb=*/nullptr, /*allow_exceptions=*/true);

    SongInfo info;
    info.songName = root.value("_songName", "");
    info.songSubName = root.value("_songSubName", "");
    info.songAuthorName = root.value("_songAuthorName", "");
    info.levelAuthorName = root.value("_levelAuthorName", "");
    info.beatsPerMinute = root.value("_beatsPerMinute", 120.0);
    info.songTimeOffset = root.value("_songTimeOffset", 0.0);
    info.previewStartTime = root.value("_previewStartTime", 12.0);
    info.previewDuration = root.value("_previewDuration", 10.0);
    info.songFilename = root.value("_songFilename", "song.egg");
    info.coverImageFilename = root.value("_coverImageFilename", "cover.jpg");
    info.environmentName = root.value("_environmentName", "DefaultEnvironment");
    info.difficultyBeatmapSets =
        ArrayFromJson<DifficultyBeatmapSet>(root, "_difficultyBeatmapSets", DifficultyBeatmapSetFromJson);

    info.extras = root;
    StripKeys(info.extras, {"_songName", "_songSubName", "_songAuthorName", "_levelAuthorName",
                            "_beatsPerMinute", "_songTimeOffset", "_previewStartTime",
                            "_previewDuration", "_songFilename", "_coverImageFilename",
                            "_environmentName", "_difficultyBeatmapSets"});
    return info;
}

std::string SerializeSongInfo(const SongInfo& info, int indent) {
    json root = info.extras;
    root["_version"] = root.value("_version", std::string("2.1.0"));
    root["_songName"] = info.songName;
    root["_songSubName"] = info.songSubName;
    root["_songAuthorName"] = info.songAuthorName;
    root["_levelAuthorName"] = info.levelAuthorName;
    root["_beatsPerMinute"] = info.beatsPerMinute;
    root["_songTimeOffset"] = info.songTimeOffset;
    root["_previewStartTime"] = info.previewStartTime;
    root["_previewDuration"] = info.previewDuration;
    root["_songFilename"] = info.songFilename;
    root["_coverImageFilename"] = info.coverImageFilename;
    root["_environmentName"] = info.environmentName;
    root["_difficultyBeatmapSets"] = ArrayToJson(info.difficultyBeatmapSets, DifficultyBeatmapSetToJson);
    return root.dump(indent);
}

SongInfo LoadSongInfoFile(const std::string& path) {
    try {
        return ParseSongInfo(ReadFile(path));
    } catch (const json::exception& e) {
        throw BeatmapIOError("Failed to parse Info.dat '" + path + "': " + e.what());
    }
}

void SaveSongInfoFile(const std::string& path, const SongInfo& info) {
    WriteFile(path, SerializeSongInfo(info));
}

namespace {
int DefaultDifficultyRank(const std::string& difficulty) {
    if (difficulty == "Easy") return 1;
    if (difficulty == "Normal") return 3;
    if (difficulty == "Hard") return 5;
    if (difficulty == "Expert") return 7;
    if (difficulty == "ExpertPlus") return 9;
    return 3;
}
} // namespace

DifficultyBeatmap& FindOrAddDifficultySlot(SongInfo& info, const std::string& characteristic,
                                           const std::string& difficulty) {
    auto setIt = std::find_if(info.difficultyBeatmapSets.begin(), info.difficultyBeatmapSets.end(),
                               [&](const DifficultyBeatmapSet& set) { return set.beatmapCharacteristicName == characteristic; });
    if (setIt == info.difficultyBeatmapSets.end()) {
        DifficultyBeatmapSet newSet;
        newSet.beatmapCharacteristicName = characteristic;
        info.difficultyBeatmapSets.push_back(std::move(newSet));
        setIt = std::prev(info.difficultyBeatmapSets.end());
    }

    auto diffIt = std::find_if(setIt->difficultyBeatmaps.begin(), setIt->difficultyBeatmaps.end(),
                                [&](const DifficultyBeatmap& d) { return d.difficulty == difficulty; });
    if (diffIt == setIt->difficultyBeatmaps.end()) {
        DifficultyBeatmap newDiff;
        newDiff.difficulty = difficulty;
        newDiff.difficultyRank = DefaultDifficultyRank(difficulty);
        newDiff.beatmapFilename = difficulty + characteristic + ".dat";
        setIt->difficultyBeatmaps.push_back(std::move(newDiff));
        diffIt = std::prev(setIt->difficultyBeatmaps.end());
    }
    return *diffIt;
}

} // namespace bs_editor::core
