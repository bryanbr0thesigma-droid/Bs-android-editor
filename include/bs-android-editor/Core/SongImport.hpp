#pragma once

#include <filesystem>
#include <string>

#include "bs-android-editor/Core/BeatmapSerializer.hpp"

// Engine-agnostic half of "create a genuinely new song from an audio
// file" (see bs_editor::hooks::CreateNewSong in EditorLauncher.cpp for the
// SongCore-facing half - resolving where new levels should live, asking
// SongCore to rescan afterward, and starting to edit the result). Kept
// separate from IL2CPP/SongCore so it's exercised by the host test suite
// like the rest of Core/, rather than only ever running for the first time
// on a real headset.

namespace bs_editor::core {

// Sanitizes `name` into a directory-name component that's safe on common
// filesystems: strips characters invalid on Windows/Android/most others
// (`/\:*?"<>|`), replacing each with '_'. Spaces and non-ASCII characters
// are left alone. Falls back to "New Song" if `name` sanitizes to empty.
std::string SanitizeFolderName(const std::string& name);

// Picks a not-yet-existing path under `root` for a new song folder, based
// on `songName` (sanitized via SanitizeFolderName): `root/songName` if
// free, otherwise `root/songName (2)`, `root/songName (3)`, etc.
std::filesystem::path PickNewSongFolder(const std::filesystem::path& root, const std::string& songName);

// Creates `levelPath`, copies `audioFile` into it as "song.egg", and
// writes a fresh Info.dat plus an empty difficulty file for a blank
// "Standard"/"ExpertPlus" slot (using FindOrAddDifficultySlot, same as
// starting an edit on an existing song's not-yet-present slot).
//
// Throws BeatmapIOError if `audioFile` doesn't exist, `levelPath` can't be
// created, the audio file can't be copied, or either file can't be
// written - callers decide what "failed" means for their own UI/logging.
void CreateNewSongFiles(const std::filesystem::path& levelPath, const std::filesystem::path& audioFile,
                        const std::string& songName, double bpm);

} // namespace bs_editor::core
