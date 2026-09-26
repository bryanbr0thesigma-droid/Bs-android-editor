# BS Android Editor

An in-headset beatmap editor for **Beat Saber on Quest (Meta's standalone
build)**: place notes, bombs, walls and lighting events without leaving VR,
undo/redo as you go, and save straight to a playable custom level — packaged
as a `.qmod` you install with QuestPatcher.

## Scope and honesty check

This repo is two things, and they're verified very differently:

1. **`include/` + `src/Core/` + `src/EditorController.cpp` + `src/EditorSession.cpp`**
   — the actual beatmap editing engine: the v3 map data model, JSON
   read/write (round-trips unknown fields untouched), grid/beat-time math,
   undo/redo, and the input-to-edit logic. This is plain C++17, has **zero**
   dependency on Beat Saber, IL2CPP, or Android, and is proven by a real,
   passing test suite you can run right now (see below).

2. **`src/Hooks/`, `src/UI/`, `qpm.json`, `mod.template.json`, `CMakeLists.txt`**
   — the Quest-side native mod that hooks into the running game and exposes
   the editor in-headset. This part is a **scaffold**: it follows the real,
   current conventions for Quest native mods (QPM, beatsaber-hook,
   custom-types, BSML), but it cannot be compiled or tested in this
   environment because doing so requires files this repo will never ship —
   see below. Files in this layer are commented with a `VERSION-SENSITIVE
   FILE` header noting exactly what to double-check against your own setup.

### Why the hook layer can't be verified here (and never should ship prebuilt)

Quest mods hook into the game's IL2CPP internals through header files
("codegen"/`bs-cordl`) that are generated *from your own installed copy of
Beat Saber* — they encode the exact class layout of that specific build, and
regenerating them requires tools like QuestPatcher pointed at your own
legally-owned APK. There is no way to obtain or fake those headers without
the game files, and redistributing anything derived from a dump of the game
itself would be a copyright problem, so this repo deliberately doesn't
attempt it. The `Hooks/`/`UI/` code here is written to compile against a
`bs-cordl` you generate yourself; you're the one who builds and signs the
final `.qmod`.

## Repo layout

```
include/bs-android-editor/
  Core/                   Data model, grid math, JSON serializer (engine-agnostic)
  EditorController.hpp    Input-frame -> document edits (engine-agnostic)
  EditorSession.hpp       Owns the active document+controller (engine-agnostic)
  Hooks/                  Declarations for the IL2CPP hooks
  UI/                     Declarations for the BSML flow coordinator/view controller
  main.hpp
src/                      Matching .cpp files for all of the above
tests/                    Standalone host-buildable test suite for the Core/ layer
resources/EditorMain.bsml BSML layout for the editor menu screen
libs/nlohmann/json.hpp    Vendored nlohmann::json (MIT), used by the Core layer
qpm.json, mod.template.json, CMakeLists.txt, build.sh   Quest/QPM build config
.github/workflows/build.yml   CI: runs the host tests, then attempts a real qmod build
```

## Running the tests (works right now, no setup)

```bash
cmake -S tests -B build-tests
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

This builds and exercises the actual editing engine: grid math, beat/time
conversion under BPM changes, `.dat`/`Info.dat` round-tripping (including
that fields this tool doesn't model, like arcs or light event box groups,
survive a load+save unchanged), undo/redo, and the controller-input-to-edit
pipeline (placing a note, dragging out a wall, deleting the nearest object).

## Building the actual Quest mod

You'll need, on your own machine (not in this sandbox):

1. **QuestPatcher** with your legally-owned Beat Saber APK, used to generate
   your own `bs-cordl` headers for your exact game version. Follow the
   BSMG modding guide for "setting up a mod project" / dumping headers if
   you haven't done this before.
2. **Android NDK** (r26 or newer) — set `ANDROID_NDK_HOME`.
3. **qpm-rust** (Quest Package Manager CLI) — install from
   [QuestPackageManager/QPM.CLI](https://github.com/QuestPackageManager/QPM.CLI).

Then:

```bash
# Pin bs-cordl to match your game version first — see qpm.json's comment.
qpm restore

./build.sh              # cross-compiles libbs-android-editor.so via CMake+NDK
qpm qmod build          # packages mod.template.json + the .so into a .qmod
                         # (if this subcommand doesn't exist in your qpm
                         # version, run `qpm --help` / `qpm qmod --help`)
```

Install the resulting `.qmod` the normal way: QuestPatcher → your Beat Saber
install → "Mod install" → pick the `.qmod` file.

`.github/workflows/build.yml` runs the host tests on every push, then
attempts this same NDK+QPM build in CI so you get a downloadable `.qmod`
artifact automatically. `qpm.json` leaves every dependency's version
unconstrained (`"*"`) so QPM's resolver can pick a mutually-compatible set
on its own — every dependency *except* `bs-cordl` tracks the modloader
ecosystem, not the game, so there's normally nothing to pin there. Only
`bs-cordl` encodes an actual Beat Saber version; leaving it at `"*"` gets
you the newest headers available, which may not match the game version you
actually have installed — pin it once you know that version (see below).

### Things you'll need to fill in before this does anything in-game

These are marked `TODO` at their exact location in the source:

- **`qpm.json`**: `bs-cordl`'s version range is a placeholder (`"*"`) —
  pin it to the version matching your dumped headers.
- **`mod.template.json`**: `author`, `packageVersion` (your exact Beat
  Saber version string), and `coverImage` are placeholders.
- **`src/UI/EditorViewController.cpp`**: populating the song list from
  installed custom levels (the community-standard way is via SongCore's
  loaded-levels API) and the "start gameplay in editor mode" transition
  (via `GlobalNamespace::MenuTransitionsHelper`, one of the classes that
  reshapes most often across game updates — check its current signature).
- **`src/Hooks/GameplayHooks.cpp`**: the controller-to-grid mapping is a
  reasonable approximation (see `kGridOriginHeightMeters`); swap in the
  game's real grid/play-space transform for pixel-perfect placement, and
  confirm the `TryGetFeatureValue` out-parameter idiom matches your
  `bs-cordl`'s codegen style (`ByRef<T>` vs plain reference).

Everything else in `Hooks/`/`UI/` is real, idiomatic Quest-modding structure
(the hook macros, the custom-types class registration pattern, the BSML
layout/button wiring) — it's the handful of spots above, plus whatever your
specific `bs-cordl` renames, that need a pass once you're building against
your own headers.

## What the editor currently supports (engine layer)

- Color notes, bombs, and full-height walls, with undo/redo for every edit
- Grid-snapped placement (4x3 grid) and 8-direction cut-angle snapping from
  thumbstick deflection, with a dot-note deadzone
- Adjustable beat-snap subdivision (1/4, 1/8, 1/16, ...)
- BPM-change-aware beat↔seconds conversion
- Load/save that never destroys map data this editor doesn't model yet
  (arcs, chains, light event box groups, `customData`, ...)

Not yet implemented (left as clear extension points, not silently missing):
partial-height walls, arcs/chains, full lighting choreography (only basic
on/off/flash events), and multi-select/box-select.
