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

} // namespace bs_editor::hooks
