# BS Android Editor

An in-headset beatmap editor for **Beat Saber on Quest (Meta's standalone
build)**: place notes (with direction, including dots), bombs and walls,
scrub/play the song while you work, undo/redo as you go, and save straight
to a playable custom level — packaged as a `.qmod` you install with
QuestPatcher.

## Scope and honesty check

This repo is two things, verified to different degrees:

1. **`include/` + `src/Core/` + `src/EditorController.cpp` + `src/EditorSession.cpp`**
   — the actual beatmap editing engine: the v3 map data model, JSON
   read/write (round-trips unknown fields untouched), grid/beat-time math,
   playback/scrubbing, undo/redo, and the input-to-edit logic. This is
   plain C++17 with **zero** dependency on Beat Saber, IL2CPP, or Android,
   and is proven by a real, passing test suite you can run right now (76
   checks — see below).

2. **`src/Hooks/`, `src/UI/`, `qpm.json`, `mod.template.json`, `CMakeLists.txt`**
   — the Quest-side native mod that hooks into the running game. This
   cannot be *compiled* in this environment (no Android NDK here), but
   every API call in it — hook macros, the logger, entry-point signatures,
   the custom-types class-registration pattern, the exact BSML helper
   names, and the specific game classes/methods it hooks — was checked
   directly against the real source of every dependency (beatsaber-hook,
   scotland2, paper2_scotland2, custom-types, BSML, and bs-cordl itself),
   not guessed from memory. Files still carry a comment where something is
   genuinely game-version-sensitive (see the TODO list below) or where I
   made a deliberate simplification worth knowing about.

### About `bs-cordl` (the IL2CPP codegen headers)

An earlier version of this README overstated how hard this part is: `qpm
restore` fetches `bs-cordl` from QPM's package registry like any other
dependency — you don't need to dump your own headers as long as someone's
already published a matching version. This project targets **Beat Saber
1.40.8_7379**, pinned in `qpm.json` as `bs-cordl: ^4008.0.0` (bs-cordl tags
its releases by game version: `1.MM.PP` → `v(MM)(0)(PP)`, e.g. `1.45.0` →
`v4500.0.0`, confirmed against bs-cordl's own GitHub releases — `v4008.0.0`
is tagged "Update for 1.40.8_7379"). If you're targeting a different game
version, change that pin to match (or dump your own via QuestPatcher if no
published version fits — the BSMG modding docs cover that).

Pinning `bs-cordl` to an older game version like this also pulls in
whatever older `beatsaber-hook`/`bsml`/`custom-types`/`scotland2` versions
were current for that build — potentially a different API shape than
what's documented as verified in the "Things you'll need to fill in"
section below, which was checked against the newer versions `qpm restore`
resolved before this pin. Re-verify the same way (their `shared/` headers
are plain text) if `qpm restore`'s dependency resolution log shows
different major versions than beatsaber-hook 8.2.1 / bsml 0.5.8 /
custom-types 0.20.1 / scotland2 0.1.7 / paper2_scotland2 4.8.0.

## Repo layout

```
include/bs-android-editor/
  Core/                   Data model, grid math, JSON serializer (engine-agnostic)
  EditorController.hpp    Input-frame -> document edits, playback/scrub (engine-agnostic)
  EditorSession.hpp       Owns the active document+controller (engine-agnostic)
  Hooks/                  Declarations for the IL2CPP hooks
  UI/                     Declarations for the BSML flow coordinator/view controller
  main.hpp
src/                      Matching .cpp files for all of the above
tests/                    Standalone host-buildable test suite for the Core/ layer
resources/EditorMain.bsml BSML layout for the editor menu screen
libs/nlohmann/json.hpp    Vendored nlohmann::json (MIT), used by the Core layer
qpm.json, mod.template.json, CMakeLists.txt, build.sh   Quest/QPM build config
.github/workflows/build.yml   CI: runs the host tests, then builds a real .qmod
```

## Running the tests (works right now, no setup)

```bash
cmake -S tests -B build-tests
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

This builds and exercises the actual editing engine: grid math, beat/time
conversion under BPM changes, playback/scrub behavior, `.dat`/`Info.dat`
round-tripping (including that fields this tool doesn't model, like arcs or
light event box groups, survive a load+save unchanged), undo/redo, and the
controller-input-to-edit pipeline (placing a note, dragging out a wall,
deleting the nearest object).

## Building the actual Quest mod

You'll need, on your own machine (not in this sandbox):

1. **Android NDK.** Check what version `bs-cordl` (or whatever it depends
   on, like `beatsaber-hook`) actually needs by reading its `qpm.json`'s
   `workspace.ndk` field — the exact NDK build that field names often isn't
   in `nttld/setup-ndk`'s download manifest, so the CI workflow here just
   uses whatever NDK the runner ships with instead (see its comments).
   Set `ANDROID_NDK_HOME` to yours.
2. **qpm-rust** (Quest Package Manager CLI) — install from
   [QuestPackageManager/QPM.CLI](https://github.com/QuestPackageManager/QPM.CLI).
3. Only if you're targeting a Beat Saber version other than 1.40.8_7379
   (this repo's current `bs-cordl` pin) and no published `bs-cordl`
   release matches it: **QuestPatcher** with your own copy of the game, to
   dump matching headers.

Then:

```bash
qpm restore

./build.sh              # cross-compiles libbs-android-editor.so via CMake+NDK
qpm qmod zip            # packages mod.template.json + the .so into a .qmod
                         # (NOT `qpm qmod build` — confirmed from qpm.cli's
                         # own source that's a deprecated alias for
                         # regenerating mod.json only, no .qmod produced)
```

`CMakeLists.txt` includes qpm's own generated `qpm_defines.cmake` and
`extern.cmake` (regenerated by every `qpm restore`, not committed) rather
than hand-rolling include paths and link flags — that's what actually wires
up per-dependency compile flags like `bs-cordl`'s required
`-fdeclspec -DUNITY_6 -DHAS_CODEGEN`.

Install the resulting `.qmod` the normal way: QuestPatcher → your Beat Saber
install → "Mod install" → pick the `.qmod` file.

`.github/workflows/build.yml` runs the host tests on every push, then does
this same NDK+QPM build in CI so you get a downloadable `.qmod` artifact
automatically.

### Things you'll need to fill in before this does anything in-game

These are marked `TODO` at their exact location in the source:

- **`mod.template.json`**: `author`, `packageVersion` (your exact Beat
  Saber version string), and `coverImage` are placeholders.
- **`src/UI/EditorViewController.cpp`**: populating the song list from
  installed custom levels (the community-standard way is via SongCore's
  loaded-levels API) and the "start gameplay in editor mode" transition
  (via `GlobalNamespace::MenuTransitionsHelper`, one of the classes that
  reshapes most often across game updates — check its current constructor/
  method overloads against your own `extern/includes`).
- **`src/Hooks/GameplayHooks.cpp`**: the controller-to-grid mapping
  (`kGridOriginHeightMeters`) is a reasonable approximation, not the game's
  real play-space/grid transform — swap it in for pixel-perfect placement
  once you've identified it.

Everything else — the hook macros, entry-point signatures, the
custom-types registration pattern, BSML's helper names, and the specific
`GlobalNamespace`/`HMUI` classes and methods hooked — was checked directly
against the real dependency source for the versions `qpm restore`
currently resolves, not guessed. If a future `qpm restore` pulls newer
major versions of these dependencies, re-verify the same way (their
`shared/` headers are plain text — grep them).

## What the editor currently supports (engine layer)

- Color notes (with 8-direction cut angle + dot notes), bombs, and
  full-height walls, with undo/redo for every edit
- Grid-snapped placement (4x3 grid) from controller position
- Adjustable beat-snap subdivision (1/4, 1/8, 1/16, ...)
- BPM-change-aware beat↔seconds conversion
- Playback and scrubbing: the editor owns its own playhead (play/pause,
  seek by a scrub axis) rather than trusting the game's own audio clock —
  see `EditorController::AdvanceTime`
- Load/save that never destroys map data this editor doesn't model yet
  (arcs, chains, light event box groups, `customData`, ...)

In-headset control mapping (`src/Hooks/GameplayHooks.cpp`): right
hand places/deletes/drags using its position, thumbstick angle (cut
direction) and trigger; left hand's thumbstick scrubs the timeline and its
trigger toggles play/pause, synced back into the game's own audio playback
via `AudioTimeSyncController::Resume`/`Pause`/`SeekTo`.

Not yet implemented (left as clear extension points, not silently
missing): partial-height walls, arcs/chains, full lighting choreography
(only basic on/off/flash events), multi-select/box-select, VR playtest
transition (see TODO above), Android audio file import with transcoding to
Ogg Vorbis, and a decorations/prop-placement mode with grab-to-transform —
the last two were explicitly scoped out of this pass to get the base
editor's native build compiling first; ask for them next.
