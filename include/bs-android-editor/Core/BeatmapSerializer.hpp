#pragma once

#include <stdexcept>
#include <string>

#include "bs-android-editor/Core/MapTypes.hpp"

namespace bs_editor::core {

class BeatmapIOError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Parses/serializes a v3 difficulty file (colorNotes/bombNotes/obstacles/
// basicBeatmapEvents/bpmEvents). Any other top-level field in the JSON is
// preserved via `BeatmapDifficulty::extras` and re-emitted untouched.
BeatmapDifficulty ParseDifficulty(const std::string& jsonText);
std::string SerializeDifficulty(const BeatmapDifficulty& difficulty, int indent = 2);

BeatmapDifficulty LoadDifficultyFile(const std::string& path);
void SaveDifficultyFile(const std::string& path, const BeatmapDifficulty& difficulty);

// Same pattern for the song's Info.dat.
SongInfo ParseSongInfo(const std::string& jsonText);
std::string SerializeSongInfo(const SongInfo& info, int indent = 2);

SongInfo LoadSongInfoFile(const std::string& path);
void SaveSongInfoFile(const std::string& path, const SongInfo& info);

} // namespace bs_editor::core
