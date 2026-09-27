#pragma once

// Resolves a SongCore-loaded level's on-disk Standard/ExpertPlus difficulty
// slot and starts the real VR gameplay scene editing it, via
// GlobalNamespace::MenuTransitionsHelper::StartStandardLevel — the same
// scene GameplayHooks.cpp's VRController/AudioTimeSyncController hooks
// already target, so once the scene switch lands, note placement/scrubbing
// just work without any further wiring.
//
// This is the least testable part of the whole mod: MenuTransitionsHelper's
// real signature was verified against bs-cordl 4008.0.0 headers, but there
// was no way to exercise the actual scene transition without a running
// game, so treat the first real-headset test of this as the real test, not
// this having compiled.
#include "songcore/shared/SongLoader/CustomBeatmapLevel.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace bs_editor::hooks {

// Starts editing `level`'s "Standard"/"ExpertPlus" difficulty slot. If that
// slot doesn't exist yet in the level's Info.dat, it's created (pointing at
// a new "ExpertPlusStandard.dat"). If `blank` is true, starts from an empty
// difficulty instead of loading whatever notes already exist for that slot
// (existing notes on disk aren't touched until the session is saved).
//
// Returns false without switching scenes if anything needed to do this
// safely couldn't be found or read (logged via this mod's Logger either
// way) - e.g. no gameplay flow coordinator has been created yet this game
// session, or the level's Info.dat/difficulty file couldn't be parsed.
bool StartEditingLevel(SongCore::SongLoader::CustomBeatmapLevel* level, bool blank);

// A genuinely new map needs its own audio file, and Quest apps can't pop
// up a normal OS file picker - so instead this folder is where a user
// drops an audio file in (via MBF/file transfer) for the mod to find.
// Created on demand if it doesn't exist yet, so it's discoverable even
// before this feature is ever used.
std::filesystem::path GetImportAudioDirectory();

// Every .ogg/.egg file currently sitting in GetImportAudioDirectory().
// Filtered to just these two extensions because Beat Saber's own
// custom-level audio loading expects Ogg Vorbis data - an .egg file is
// just an Ogg Vorbis file under a different extension, by convention -
// so an arbitrary .mp3/.wav dropped in there wouldn't play right even if
// listed here.
std::vector<std::filesystem::path> ListImportableAudioFiles();

// Creates a brand new custom level folder under SongCore's own preferred
// custom-level path from `audioFile` (copied in as "song.egg"), a fresh
// Info.dat naming a blank "Standard"/"ExpertPlus" difficulty, and an
// empty difficulty file for that slot. Returns false immediately if the
// audio file or the new folder itself couldn't be set up.
//
// SongCore doesn't know this level exists until it rescans, so this kicks
// off that rescan and returns without starting to edit anything yet -
// call PollPendingNewSong() once a frame (already wired into
// GameplayHooks.cpp's VRController_Update hook) to pick up when the
// rescan finishes and actually start editing the new level.
bool CreateNewSong(const std::filesystem::path& audioFile, const std::string& songName, double bpm);

// Checks whether a CreateNewSong() rescan has finished and, if so, starts
// editing the new level (the same way StartEditingLevel does). A no-op
// when no CreateNewSong() call is pending, so this is safe to call
// unconditionally every frame.
void PollPendingNewSong();

} // namespace bs_editor::hooks
