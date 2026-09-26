#include "bs-android-editor/Core/EditorDocument.hpp"

#include <functional>
#include <utility>

namespace bs_editor::core {

namespace {

class LambdaCommand final : public ICommand {
public:
    LambdaCommand(std::function<void()> doFn, std::function<void()> undoFn)
        : doFn_(std::move(doFn)), undoFn_(std::move(undoFn)) {}

    void Do() override { doFn_(); }
    void Undo() override { undoFn_(); }

private:
    std::function<void()> doFn_;
    std::function<void()> undoFn_;
};

} // namespace

EditorDocument::EditorDocument(BeatmapDifficulty difficulty, SongInfo info)
    : difficulty_(std::move(difficulty)), info_(std::move(info)) {}

void EditorDocument::Execute(std::unique_ptr<ICommand> command) {
    command->Do();
    undoStack_.push_back(std::move(command));
    redoStack_.clear();
}

std::size_t EditorDocument::AddColorNote(ColorNote note) {
    const std::size_t index = difficulty_.colorNotes.size();
    EditorDocument* self = this;
    Execute(std::make_unique<LambdaCommand>(
        [self, note]() { self->difficulty().colorNotes.push_back(note); },
        [self]() { self->difficulty().colorNotes.pop_back(); }));
    return index;
}

void EditorDocument::RemoveColorNoteAt(std::size_t index) {
    if (index >= difficulty_.colorNotes.size()) {
        throw std::out_of_range("RemoveColorNoteAt: index out of range");
    }
    const ColorNote removed = difficulty_.colorNotes[index];
    EditorDocument* self = this;
    Execute(std::make_unique<LambdaCommand>(
        [self, index]() {
            auto& notes = self->difficulty().colorNotes;
            notes.erase(notes.begin() + static_cast<std::ptrdiff_t>(index));
        },
        [self, index, removed]() {
            auto& notes = self->difficulty().colorNotes;
            notes.insert(notes.begin() + static_cast<std::ptrdiff_t>(index), removed);
        }));
}

void EditorDocument::MoveColorNote(std::size_t index, double newBeat, int newLineIndex, int newLineLayer) {
    if (index >= difficulty_.colorNotes.size()) {
        throw std::out_of_range("MoveColorNote: index out of range");
    }
    const ColorNote before = difficulty_.colorNotes[index];
    ColorNote after = before;
    after.b = newBeat;
    after.x = newLineIndex;
    after.y = newLineLayer;

    EditorDocument* self = this;
    Execute(std::make_unique<LambdaCommand>(
        [self, index, after]() { self->difficulty().colorNotes[index] = after; },
        [self, index, before]() { self->difficulty().colorNotes[index] = before; }));
}

std::size_t EditorDocument::AddBombNote(BombNote bomb) {
    const std::size_t index = difficulty_.bombNotes.size();
    EditorDocument* self = this;
    Execute(std::make_unique<LambdaCommand>(
        [self, bomb]() { self->difficulty().bombNotes.push_back(bomb); },
        [self]() { self->difficulty().bombNotes.pop_back(); }));
    return index;
}

void EditorDocument::RemoveBombNoteAt(std::size_t index) {
    if (index >= difficulty_.bombNotes.size()) {
        throw std::out_of_range("RemoveBombNoteAt: index out of range");
    }
    const BombNote removed = difficulty_.bombNotes[index];
    EditorDocument* self = this;
    Execute(std::make_unique<LambdaCommand>(
        [self, index]() {
            auto& bombs = self->difficulty().bombNotes;
            bombs.erase(bombs.begin() + static_cast<std::ptrdiff_t>(index));
        },
        [self, index, removed]() {
            auto& bombs = self->difficulty().bombNotes;
            bombs.insert(bombs.begin() + static_cast<std::ptrdiff_t>(index), removed);
        }));
}

std::size_t EditorDocument::AddObstacle(Obstacle obstacle) {
    const std::size_t index = difficulty_.obstacles.size();
    EditorDocument* self = this;
    Execute(std::make_unique<LambdaCommand>(
        [self, obstacle]() { self->difficulty().obstacles.push_back(obstacle); },
        [self]() { self->difficulty().obstacles.pop_back(); }));
    return index;
}

void EditorDocument::RemoveObstacleAt(std::size_t index) {
    if (index >= difficulty_.obstacles.size()) {
        throw std::out_of_range("RemoveObstacleAt: index out of range");
    }
    const Obstacle removed = difficulty_.obstacles[index];
    EditorDocument* self = this;
    Execute(std::make_unique<LambdaCommand>(
        [self, index]() {
            auto& obstacles = self->difficulty().obstacles;
            obstacles.erase(obstacles.begin() + static_cast<std::ptrdiff_t>(index));
        },
        [self, index, removed]() {
            auto& obstacles = self->difficulty().obstacles;
            obstacles.insert(obstacles.begin() + static_cast<std::ptrdiff_t>(index), removed);
        }));
}

std::size_t EditorDocument::AddBasicEvent(BasicEvent event) {
    const std::size_t index = difficulty_.basicBeatmapEvents.size();
    EditorDocument* self = this;
    Execute(std::make_unique<LambdaCommand>(
        [self, event]() { self->difficulty().basicBeatmapEvents.push_back(event); },
        [self]() { self->difficulty().basicBeatmapEvents.pop_back(); }));
    return index;
}

void EditorDocument::RemoveBasicEventAt(std::size_t index) {
    if (index >= difficulty_.basicBeatmapEvents.size()) {
        throw std::out_of_range("RemoveBasicEventAt: index out of range");
    }
    const BasicEvent removed = difficulty_.basicBeatmapEvents[index];
    EditorDocument* self = this;
    Execute(std::make_unique<LambdaCommand>(
        [self, index]() {
            auto& events = self->difficulty().basicBeatmapEvents;
            events.erase(events.begin() + static_cast<std::ptrdiff_t>(index));
        },
        [self, index, removed]() {
            auto& events = self->difficulty().basicBeatmapEvents;
            events.insert(events.begin() + static_cast<std::ptrdiff_t>(index), removed);
        }));
}

void EditorDocument::Undo() {
    if (undoStack_.empty()) return;
    auto command = std::move(undoStack_.back());
    undoStack_.pop_back();
    command->Undo();
    redoStack_.push_back(std::move(command));
}

void EditorDocument::Redo() {
    if (redoStack_.empty()) return;
    auto command = std::move(redoStack_.back());
    redoStack_.pop_back();
    command->Do();
    undoStack_.push_back(std::move(command));
}

void EditorDocument::ClearHistory() {
    undoStack_.clear();
    redoStack_.clear();
}

} // namespace bs_editor::core
