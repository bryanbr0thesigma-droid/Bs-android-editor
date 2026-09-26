#include "bs-android-editor/EditorSession.hpp"

#include <utility>

namespace bs_editor {

EditorSession& EditorSession::Instance() {
    static EditorSession instance;
    return instance;
}

void EditorSession::Start(core::SongInfo info, core::BeatmapDifficulty difficulty, double baseBpm,
                           int snapSubdivision) {
    document_ = std::make_unique<core::EditorDocument>(std::move(difficulty), std::move(info));
    controller_ = std::make_unique<EditorController>(*document_, baseBpm, snapSubdivision);
}

void EditorSession::End() {
    controller_.reset();
    document_.reset();
}

} // namespace bs_editor
