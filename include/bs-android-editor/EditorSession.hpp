#pragma once

#include <memory>
#include <string>

#include "bs-android-editor/Core/EditorDocument.hpp"
#include "bs-android-editor/Core/MapTypes.hpp"
#include "bs-android-editor/EditorController.hpp"

// The single active in-headset editing session, if any. Deliberately
// engine-agnostic (owns only the core document + controller) so it stays
// host-testable; src/Hooks/GameplayHooks.cpp and src/UI/* are the only
// places that reach into it from game/UI code.

namespace bs_editor {

class EditorSession {
public:
    static EditorSession& Instance();

    bool IsActive() const { return document_ != nullptr; }

    // Starts a new session for the given song/difficulty. Replaces any
    // session already in progress (callers should prompt to save first).
    // songInfoPath/difficultyPath are where Save() writes to; leave empty
    // for a session that's never meant to be saved to disk (e.g. tests).
    void Start(core::SongInfo info, core::BeatmapDifficulty difficulty, double baseBpm,
               int snapSubdivision = 8, std::string songInfoPath = {}, std::string difficultyPath = {});
    void End();

    core::EditorDocument* document() { return document_.get(); }
    EditorController* controller() { return controller_.get(); }

    // Writes the current document back to the paths given to Start().
    // No-ops (returns false) if there's no active session or no
    // difficultyPath was given to Start().
    bool Save() const;

    const std::string& difficultyFilePath() const { return difficultyFilePath_; }
    const std::string& songInfoFilePath() const { return songInfoFilePath_; }

private:
    std::unique_ptr<core::EditorDocument> document_;
    std::unique_ptr<EditorController> controller_;
    std::string songInfoFilePath_;
    std::string difficultyFilePath_;
};

} // namespace bs_editor
