#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

#include "bs-android-editor/Core/MapTypes.hpp"

// In-memory editable beatmap with a linear undo/redo history. This is the
// single source of truth the in-headset editor mutates; every mutating
// method here is undoable. Deliberately engine-agnostic (no IL2CPP, no VR
// input) so it can be exercised by host-side tests.
//
// EditorDocument is non-copyable and non-movable: its undo/redo commands
// close over `this`, so the document's address must stay stable for the
// lifetime of its history.

namespace bs_editor::core {

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Do() = 0;
    virtual void Undo() = 0;
};

class EditorDocument {
public:
    explicit EditorDocument(BeatmapDifficulty difficulty = {}, SongInfo info = {});

    EditorDocument(const EditorDocument&) = delete;
    EditorDocument& operator=(const EditorDocument&) = delete;
    EditorDocument(EditorDocument&&) = delete;
    EditorDocument& operator=(EditorDocument&&) = delete;

    const BeatmapDifficulty& difficulty() const { return difficulty_; }
    BeatmapDifficulty& difficulty() { return difficulty_; }

    const SongInfo& info() const { return info_; }
    SongInfo& info() { return info_; }

    // Each call below performs one undoable edit and returns the index of
    // the affected element (for Add*) or throws std::out_of_range for a
    // bad index (for Remove*/Move*).
    std::size_t AddColorNote(ColorNote note);
    void RemoveColorNoteAt(std::size_t index);
    void MoveColorNote(std::size_t index, double newBeat, int newLineIndex, int newLineLayer);

    std::size_t AddBombNote(BombNote bomb);
    void RemoveBombNoteAt(std::size_t index);

    std::size_t AddObstacle(Obstacle obstacle);
    void RemoveObstacleAt(std::size_t index);

    std::size_t AddBasicEvent(BasicEvent event);
    void RemoveBasicEventAt(std::size_t index);

    bool CanUndo() const { return !undoStack_.empty(); }
    bool CanRedo() const { return !redoStack_.empty(); }
    void Undo();
    void Redo();
    void ClearHistory();
    std::size_t UndoStackSize() const { return undoStack_.size(); }

private:
    void Execute(std::unique_ptr<ICommand> command);

    BeatmapDifficulty difficulty_;
    SongInfo info_;
    std::vector<std::unique_ptr<ICommand>> undoStack_;
    std::vector<std::unique_ptr<ICommand>> redoStack_;
};

} // namespace bs_editor::core
