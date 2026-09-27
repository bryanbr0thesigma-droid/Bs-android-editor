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
   and is proven by a real, passing test suite you can run right now (104
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
were current for that build — a genuinely different API shape in a couple
of places, not just a version bump. For 1.40.8_7379, `qpm restore`
currently resolves beatsaber-hook 6.4.2 / bsml 0.4.55 / custom-types 0.18.4
/ scotland2 0.1.7 / paper2_scotland2 4.8.0, and every hook/UI file in this
repo has been re-verified against that exact set (not the newer
beatsaber-hook 8.2.1 / bsml 0.5.8 / custom-types 0.20.1 set an earlier pass
checked before this pin). The two differences that actually mattered:
beatsaber-hook 6.4.2 keeps its headers under a `shared/utils/` subfolder
(e.g. `beatsaber-hook/shared/utils/hooking.hpp`, `.../il2cpp-functions.hpp`
with `il2cpp_functions::Init()`) instead of the flat `shared/*.hpp` layout
8.2.1 moved to; and bsml 0.4.55 has no `BSML::BSMLViewController`
convenience base at all, so `EditorViewController` derives directly from
`HMUI::ViewController` and calls
`BSML::BSMLParser::parse_and_construct(str, parent, host)` by hand from its
own `DidActivate` override — the same pattern bsml's own
`BSML::MenuButtonsViewController` uses internally at this version. If a
future `qpm restore` resolves yet another set, re-verify the same way
(their `shared/` headers are plain text — grep them for the class/macro
you're about to call).

**`qpm.json`'s `versionRange: "*"` for beatsaber-hook/custom-types/bsml/
paper2_scotland2/scotland2** is permissive on purpose (worked around an
unrelated resolver error early on) — confirmed harmless (see below).

**Launch crash, root cause and fix (resolved):** early builds crashed the
whole game on launch, before the main menu ever appeared, as soon as this
mod linked against SongCore. Dependency-version mismatches, `mod.json`
packaging, and a couple of specific-looking static-initializer theories
were all checked and ruled out with hard evidence (exact version matches
confirmed on a real headset via MBF's installed-mods list; a clean CI
link with no undefined symbols; a diagnostic build that dropped the
`songcore` dependency entirely reached the main menu fine; a follow-up
build that re-added `songcore` as a dependency with **zero code** calling
into it crashed again, proving the problem was below the API level,
purely at linking/loading). The actual cause: SongCore v1.1.26 needs three
**private** transitive libraries to link — `lapiz ^0.2.21`, `kaleb
^0.1.9`, `libcryptopp ^8.5.0`. `qpm restore` fetches these for this
project's own build too, so the `.so` links cleanly, but qpm's packaging
logic only bundles a dependency's `.so` into this mod's `.qmod` when it's
**directly** listed in *this* project's own `qpm.json` (see `package.rs`'s
`to_mod_json`, specifically the `direct_dependencies.contains(...)`
filter on `libraryFiles`) — a dependency pulled in only transitively
through `songcore` doesn't qualify, regardless of whether the compiled
`.so` needs it at runtime. A real, popular SongCore-dependent mod
([BetterSongSearchQuest](https://github.com/bsq-ports/BetterSongSearchQuest))
works around exactly this by also directly declaring `kaleb` itself
(`private: true`, since it doesn't use kaleb's API, just needs the `.so`
bundled) — this project never did that for `lapiz`/`kaleb`/`libcryptopp`.
All three are now direct dependencies here too, matching SongCore
1.1.26's exact version ranges, and this was confirmed on a real headset to
fix the crash.

Separately, and unrelated to the crash: `MenuHooks.cpp`'s main-menu button
injection and `main.cpp`'s `load()` both got try/catch + step-by-step
logging, and `EditorFlowCoordinator` got a `BackButtonWasPressed`
override it was missing (it showed a back button that didn't actually do
anything — every `HMUI::FlowCoordinator` is expected to override this and
dismiss itself, confirmed against bsml 0.4.55's own
`MainMenuHolderFlowCoordinator`). Both were found and fixed in the course
of the same round of real-headset testing.

If you hit "failed to import"/"agent responded with an error" in MBF
importing a `.qmod` fetched from a CI artifact link: GitHub Actions always
wraps a downloaded workflow artifact in an outer zip, and importing that
outer wrapper produces exactly this generic-sounding error. Extract the
outer zip and import the `.qmod` file inside it, not the wrapper itself.

### Song selection and the gameplay-scene transition

"New Blank Map"/"Edit Selected" resolve the picked song via
[SongCore](https://github.com/raineaeternal/Quest-SongCore) (pinned in
`qpm.json` as `songcore: ^1.1.24`, the exact tag bumped for 1.40.8_7379,
confirmed the same way as `bs-cordl` above) rather than reading the
filesystem by hand: `SongCore::API::Loading::GetAllLevels()` for the list,
and each entry is already a `GlobalNamespace::BeatmapLevel` subclass
(`SongCore::SongLoader::CustomBeatmapLevel`), so it can be passed straight
into the game's own level-start API with no conversion.

**You need SongCore installed separately before installing this mod.**
It's a normal (not auto-installing) dependency in `qpm.json` — install it
yourself first the normal way (from its GitHub releases, or via your
installer's own core-mods list if it offers SongCore there).

**"Edit Selected"** works on a selected **existing** song's
"Standard"/"ExpertPlus" difficulty slot (creating that slot in `Info.dat`
if it doesn't exist yet), loading whatever's already saved there.

**"New Blank Map"** is the entry point for a genuinely new song, not a new
difficulty on an existing one (an earlier version of this button did the
latter — the first real-headset test of it made clear that wasn't what
"New Blank Map" should mean). Clicking it swaps the same list over to show
audio files instead of installed songs — specifically, `.ogg`/`.egg` files
sitting in
`/sdcard/ModData/com.beatgames.beatsaber/Mods/bs-android-editor/ImportAudio/`.
That folder is created during `load()` in `main.cpp` — unconditionally, at
mod load time, not lazily the first time you click the button — so it's
there to drop files into via MBF/file transfer as soon as the mod is
installed, before you ever open the editor screen. (An earlier version of
this created the folder lazily on first click instead, which was the
actual cause of the first real-headset report of this feature: dropping a
file in before ever clicking "New Blank Map" meant the folder didn't
exist yet.) Re-clicking "New Blank Map" while already in this mode just
rescans the folder, so dropping in a new file doesn't need leaving the
screen. If the folder is empty, the list shows a single line saying so
rather than just looking empty/broken.

A song-name field, a BPM stepper, and a **"Create & Edit New Song"**
button all appear too — all three created in C++ via `BSML::Lite::
CreateStringSetting`/`CreateIncrementSetting`/`CreateUIButton`
(`EditorViewController.cpp`), not BSML markup, since those helpers take a
plain callback with no markup value-binding to get wrong; their exact
on-screen position is a first guess, not something measured against a
running game, so nudge the anchored positions there if they land somewhere
awkward. (An earlier version reused the markup "Edit Selected" button as
the confirm action without actually relabeling it, which meant there was
no visible way to confirm an import at all — also caught by the first
real-headset test, hence the dedicated button now; "Edit Selected" is
back to only ever meaning "edit an existing selected song".) Only
`.ogg`/`.egg` are accepted because Beat Saber's own custom-level audio
loading expects Ogg Vorbis data — an `.egg` file is just an Ogg Vorbis
file under a different extension, by convention, so an `.mp3`/`.wav`
dropped in that folder won't play right even though nothing stops you
from renaming one; convert it to Ogg Vorbis first (outside this mod) if
that's what you're starting from.

Selecting a file and clicking **"Create & Edit New Song"** copies that
file into a new folder under SongCore's own preferred custom-level path
as `song.egg`, writes a fresh `Info.dat` and an empty difficulty file for
it, asks SongCore to rescan (`SongCore::API::Loading::RefreshSongs`), and
— once that finishes, polled non-blockingly once a frame rather than
blocked on synchronously, since `RefreshSongs`'s own doc comment implies
its completion is meant to be observed via an event/future rather than
waited on — starts editing the new level the same way "Edit Selected" on
an existing song does. There's no on-screen progress indicator for that
wait yet, just a delay before the scene changes
(`src/Hooks/EditorLauncher.cpp`'s `CreateNewSong`/`PollPendingNewSong`).
Leaving this screen (back button) and reopening it always resets back to
the normal existing-song list — that's the only way out of import mode
right now, there's no separate cancel button.

`src/Hooks/EditorLauncher.cpp` is the piece that actually switches into the
real VR gameplay scene (the same one `GameplayHooks.cpp`'s
`VRController`/`AudioTimeSyncController` hooks already target) via
`GlobalNamespace::MenuTransitionsHelper::StartStandardLevel` — by a wide
margin the most complex single call in this codebase (19 parameters). Two
things about *how* it gets there are worth knowing if it misbehaves:

- `MenuTransitionsHelper` and the `EnvironmentsListModel` it needs are
  fetched via `UnityEngine::Resources::FindObjectsOfTypeAll` rather than
  through Zenject DI (which raw hooks like this can't easily reach into).
  This is not a guess — BSML's own `ModSettingsFlowCoordinator` fetches
  `MenuTransitionsHelper` the identical way in its real 0.4.55 source. The
  `EnvironmentsListModel` comes off an existing
  `SinglePlayerLevelSelectionFlowCoordinator`'s already-injected field
  (verified field names against bs-cordl 4008.0.0's header). Both require
  you to have opened Solo Play at least once already this game session —
  that's when the game itself first creates them; there's currently no
  fallback if you haven't.
- Every other parameter (`GameplayModifiers`, `PlayerSpecificSettings`,
  etc.) uses that type's own parameterless default constructor, and
  `ColorScheme`/`OverrideEnvironmentSettings`/`PracticeSettings` are passed
  as `nullptr` (meaning "no override" / "not practice mode") rather than
  constructed, both to reduce the number of things that could be wrong and
  because the game's own defaults are what you want here anyway.

This is the one part of the whole mod that couldn't be exercised at all
before landing — there's no way to run IL2CPP call sites outside a real
game process. Every signature was checked against the real bs-cordl
4008.0.0 headers, but treat your first real-headset test of "Edit
Selected" on an existing song's existing difficulty (the closest thing to
a controlled test of `StartStandardLevel` itself, without also depending
on a difficulty slot or brand new song folder actually being valid) as the
actual test, not this having compiled in CI. The first real attempt at
this (via the old "New Blank Map" behavior, editing a blank difficulty
that didn't already exist for that song) froze briefly then did nothing —
consistent with `StartStandardLevel` silently rejecting a
characteristic/difficulty combination SongCore's own live model of that
level didn't actually have, since the difficulty slot only existed in this
mod's own in-memory copy of `Info.dat`, never written to disk or told to
SongCore. Both "Edit Selected" on an existing difficulty and the new
"New Blank Map" import flow (which does write to disk and does tell
SongCore, via `RefreshSongs`, before attempting the transition) are meant
to avoid that specific failure — neither has been confirmed on a real
headset yet as of this paragraph.

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
light event box groups, survive a load+save unchanged), undo/redo,
tool/color/snap cycling, session save-to-disk, and the
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

One packaging quirk worth knowing about: `beatsaber-hook`'s published
`libbeatsaber-hook.so` deliberately does **not** export `A64HookFunction`
(it's compiled with hidden visibility — presumably so multiple mods
loaded into the same process each get their own private copy instead of
fighting over one shared one), even though the hook-installation macros
every mod uses (`MAKE_HOOK_MATCH`/`INSTALL_HOOK`) call it directly from a
header template, so it has to be compiled fresh into *your* mod's `.so`
too (leaving it out compiles fine but fails to *link*, with `ld.lld:
error: undefined symbol: A64HookFunction`). `CMakeLists.txt` handles this
with a plain `file(GLOB ...)` + `target_sources()` over
`extern/includes/beatsaber-hook/shared/inline-hook/*.{c,cpp}`, right after
the `extern.cmake` include.

That's deliberately **not** done via qpm's own `additionalData.extraFiles`
mechanism (which exists for exactly this kind of case): that mechanism
tries to symlink `shared/inline-hook` a *second* time on top of the
directory symlink that already exposes all of `beatsaber-hook`'s `shared/`
tree — inline-hook included — as part of its normal headers. The
resulting "File exists" fallback copy resolves both sides of the copy to
the same inode through the nested symlinks and silently truncates
`And64InlineHook.hpp`/`.cpp` to 0 bytes (reproduced locally by walking
through `qpm.cli`'s own `collect_deps`/`copy_from_cache` source) — which
then fails with a *different*, more confusing error
(`use of undeclared identifier 'A64HookFunction'`) despite the include
line resolving "successfully". Since the files are already present via
the ordinary header restore, no `qpm.json` change is needed at all — just
compile them directly.

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

- **`mod.template.json`**: `author` is a placeholder. `packageVersion` is
  already set to this repo's pinned target (`1.40.8_7379`) — change it if
  you re-pin `bs-cordl` to a different game version. There's no
  `coverImage` yet (it's optional — QuestPatcher just shows a default
  icon without one); add one and a `"coverImage": "cover.png"` entry
  (plus listing it in `qmodIncludeDirs`-searchable location) if you want
  a custom one.
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

- Color notes (with 8-direction cut angle + dot notes), bombs, full-height
  walls, and basic (on/off/flash) lighting events, with undo/redo for
  every edit
- Grid-snapped placement (4x3 grid) from controller position
- Adjustable beat-snap subdivision, cycled in-headset: 1/4, 1/8, 1/16, 1/32
- BPM-change-aware beat↔seconds conversion
- Playback and scrubbing: the editor owns its own playhead (play/pause,
  seek by a scrub axis) rather than trusting the game's own audio clock —
  see `EditorController::AdvanceTime`
- Load/save that never destroys map data this editor doesn't model yet
  (arcs, chains, light event box groups, `customData`, ...) — `Save()`
  writes both the difficulty file and `Info.dat` back to the paths the
  session was started with

### In-headset controls (`src/Hooks/GameplayHooks.cpp`)

Right hand places/deletes/drags using its position, thumbstick angle (cut
direction) and trigger — mirrors the base game's saber hand for cutting
notes. Left hand's thumbstick scrubs the timeline and its trigger toggles
play/pause, synced back into the game's own audio playback via
`AudioTimeSyncController::Resume`/`Pause`/`SeekTo`.

Everything else (switching what the trigger places, undo/redo, save) is
mapped to face buttons and thumbstick clicks — deliberately not gated
behind any menu — via `GlobalNamespace::OVRInput` (Oculus Integration's own
input API; still present and working in this bs-cordl dump, confirmed
against its real `Get(Down)`/`OVRInput_Button`/`OVRInput_Controller`
members, which `VRController` itself doesn't expose):

| Button | Hand | Action |
|---|---|---|
| A (`One`) | Right | Cycle tool: Note → Bomb → Wall → Light → Delete → ... |
| B (`Two`) | Right | Cycle active note color (Red/Blue) |
| Thumbstick click | Right | Cycle beat-snap subdivision |
| X (`Three`) | Left | Undo |
| Y (`Four`) | Left | Redo |
| Thumbstick click | Left | Save to disk |

Not yet implemented (left as clear extension points, not silently
missing): partial-height walls, arcs/chains, full lighting choreography
(only basic on/off/flash events), multi-select/box-select, a
difficulty-slot picker (everything currently targets a single fixed
"Standard"/"ExpertPlus" slot — see the difficulty parameter hardcoded in
`EditorLauncher.cpp`), audio transcoding on import (see "New Blank Map"
above — only `.ogg`/`.egg` are accepted, anything else needs converting to
Ogg Vorbis outside this mod first), an on-screen progress indicator for
the SongCore-rescan delay after importing a new song, and a
decorations/prop-placement mode with grab-to-transform — ask for any of
these next.
