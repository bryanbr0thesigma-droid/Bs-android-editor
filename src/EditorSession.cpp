#include "bs-android-editor/EditorSession.hpp"

#include <utility>

#include "bs-android-editor/Core/BeatmapSerializer.hpp"

namespace bs_editor {

EditorSession& EditorSession::Instance() {
    static EditorSession instance;
    return instance;
}

void EditorSession::Start(core::SongInfo info, core::BeatmapDifficulty difficulty, double baseBpm,
                           int snapSubdivision, std::string songInfoPath, std::string difficultyPath) {
    document_ = std::make_unique<core::EditorDocument>(std::move(difficulty), std::move(info));
    controller_ = std::make_unique<EditorController>(*document_, baseBpm, snapSubdivision);
    songInfoFilePath_ = std::move(songInfoPath);
    difficultyFilePath_ = std::move(difficultyPath);
}

void EditorSession::End() {
    controller_.reset();
    document_.reset();
    songInfoFilePath_.clear();
    difficultyFilePath_.clear();
}

bool EditorSession::Save() const {
    if (!IsActive() || difficultyFilePath_.empty()) return false;
    core::SaveDifficultyFile(difficultyFilePath_, document_->difficulty());
    if (!songInfoFilePath_.empty()) {
        core::SaveSongInfoFile(songInfoFilePath_, document_->info());
    }
    return true;
}

} // namespace bs_editor
