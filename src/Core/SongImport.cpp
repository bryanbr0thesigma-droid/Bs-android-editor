#include "bs-android-editor/Core/SongImport.hpp"

#include <system_error>

namespace bs_editor::core {

namespace {
constexpr const char* kCharacteristic = "Standard";
constexpr const char* kDifficulty = "ExpertPlus";
} // namespace

std::string SanitizeFolderName(const std::string& name) {
    std::string result = name;
    for (char& c : result) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' ||
            c == '|') {
            c = '_';
        }
    }
    if (result.empty()) result = "New Song";
    return result;
}

std::filesystem::path PickNewSongFolder(const std::filesystem::path& root, const std::string& songName) {
    const std::string folderName = SanitizeFolderName(songName);
    std::filesystem::path path = root / folderName;

    std::error_code ec;
    for (int suffix = 2; std::filesystem::exists(path, ec); ++suffix) {
        path = root / (folderName + " (" + std::to_string(suffix) + ")");
    }
    return path;
}

void CreateNewSongFiles(const std::filesystem::path& levelPath, const std::filesystem::path& audioFile,
                        const std::string& songName, double bpm) {
    std::error_code ec;
    if (!std::filesystem::exists(audioFile, ec) || ec) {
        throw BeatmapIOError("CreateNewSongFiles: audio file '" + audioFile.string() + "' doesn't exist");
    }

    std::filesystem::create_directories(levelPath, ec);
    if (ec) {
        throw BeatmapIOError("CreateNewSongFiles: couldn't create '" + levelPath.string() + "': " + ec.message());
    }

    std::filesystem::copy_file(audioFile, levelPath / "song.egg", ec);
    if (ec) {
        throw BeatmapIOError("CreateNewSongFiles: couldn't copy audio into '" + levelPath.string() +
                              "': " + ec.message());
    }

    SongInfo info;
    info.songName = songName;
    info.songAuthorName = "Unknown";
    info.levelAuthorName = "BS Android Editor";
    info.beatsPerMinute = bpm;
    info.songFilename = "song.egg";
    DifficultyBeatmap& slot = FindOrAddDifficultySlot(info, kCharacteristic, kDifficulty);

    SaveSongInfoFile((levelPath / "Info.dat").string(), info);
    SaveDifficultyFile((levelPath / slot.beatmapFilename).string(), BeatmapDifficulty{});
}

} // namespace bs_editor::core
