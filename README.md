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
unrelated resolver error early on) and, despite looking sloppy, turned out
NOT to be the cause of a real problem it was suspected of: a launch crash
with zero log output, tested against a headset whose installed versions
of all four (beatsaber-hook 6.4.2, bsml 0.4.55, custom-types 0.18.4,
paper2_scotland2 4.8.0) matched exactly what this repo compiles against
anyway. Tightening these to `^<resolved version>` didn't fix that crash
and broke something else (see below), so it's back to `"*"` here.

**`songcore`'s `additionalData.includeQmod: false`** is the one dependency
flag in this file that *is* load-bearing, and non-obviously so: without
it, `qpm qmod zip`-generated `mod.json` includes a `songcore` entry in its
`dependencies` array, and — isolated by testing four separate builds
against a real headset (two different `songcore` version-range strings,
one identical to the other, plus a build with no `songcore` reference at
all) — every `.qmod` with that entry present consistently failed to even
*import* into MBF at all ("Agent responded with an error", no further
detail available), regardless of the version string used, while the one
build without it imported fine. The underlying cause (something in how
MBF resolves `songcore`'s own nested dependency chain against this mod's,
a bug in MBF, or something else) isn't confirmed — this just routes
around it the same way `scotland2` already had to be routed around,
trading away this installer's ability to auto-install SongCore for anyone
who doesn't already have it (document that as a manual prerequisite until
this gets root-caused properly). The `.so` still links against and calls
`libsongcore.so` identically either way; this flag only controls whether
`mod.json` *declares* it as a fetchable dependency.

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
It's `includeQmod: false` in `qpm.json` (see the note above) so QuestPatcher/
MBF won't auto-install it for you — install it yourself first the normal
way (from its GitHub releases, or via your installer's own core-mods list
if it offers SongCore there).

Both buttons always work on a **selected song's** "Standard"/"ExpertPlus"
difficulty slot (creating that slot in `Info.dat` if it doesn't exist yet)
— "New Blank Map" starts that slot from an empty difficulty instead of
loading whatever's already saved there, it does not create a new song from
nothing. That's a direct consequence of song import not being implemented
yet (see the bottom of this file): a new song folder with no audio file
would never load, so there's currently no way to create a map for
audio that isn't already an installed custom level's.

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
4008.0.0 headers, but treat your first real-headset test of "New Blank
Map"/"Edit Selected" as the actual test, not this having compiled in CI.

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
`EditorLauncher.cpp`), Android audio file import with transcoding to Ogg
Vorbis (which is also what's blocking "New Blank Map" from creating a
genuinely new song — see above), and a decorations/prop-placement mode
with grab-to-transform — ask for any of these next.
