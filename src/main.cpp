// Mod entry point for the Scotland2 modloader + beatsaber-hook, targeting
// Beat Saber 1.40.8_7379 (see qpm.json: bs-cordl ^4008.0.0).
//
// setup(CModInfo*)/load() match modloader::SetupFunc/LoadFunc exactly (see
// scotland2's shared/loader.hpp — scotland2 0.1.7 resolves the same
// regardless of the beatsaber-hook major version pinned below).
//
// At the beatsaber-hook version this project's dependencies resolve to for
// 1.40.8 (6.4.2), il2cpp_functions::Init() (declared in
// beatsaber-hook/shared/utils/il2cpp-functions.hpp) is what needs to run
// once il2cpp is up — the newer i2c::functions::initialize()/api.hpp only
// exist from beatsaber-hook ~7.x onward. Re-check this against your own
// extern/includes if a future `qpm restore` resolves a newer major version.
#include "bs-android-editor/main.hpp"
#include "bs-android-editor/Hooks/EditorLauncher.hpp"
#include "bs-android-editor/Hooks/GameplayHooks.hpp"
#include "bs-android-editor/Hooks/MenuHooks.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "scotland2/shared/modloader.h"

#include <exception>

extern "C" void setup(CModInfo* info) {
    info->id = MOD_ID;
    info->version = VERSION;
    info->version_long = 0;
    Logger.info("bs-android-editor setup complete");
}

// Each install call is wrapped separately so a metadata-resolution failure
// in one hook (e.g. from an unexpected game build) gets logged and skips
// just that hook, rather than throwing out of load() uncaught and taking
// the whole game process down with it before any of this ever reaches a
// log file - which is what an unguarded failure here would otherwise look
// like from the outside: a crash with no trace of bs-android-editor ever
// having run.
extern "C" void load() {
    Logger.info("bs-android-editor load() starting");

    // Plain filesystem work, no il2cpp needed - done first and
    // unconditionally so the folder genuinely exists before the player
    // ever opens the editor screen, rather than only being created
    // lazily the first time "New Blank Map" is clicked (which meant a
    // file dropped in beforehand could land in a folder that didn't
    // exist yet).
    try {
        const auto importDir = bs_editor::hooks::GetImportAudioDirectory();
        Logger.info("Import-audio folder ready at {}", importDir.string());
    } catch (const std::exception& e) {
        Logger.error("Failed to set up the import-audio folder: {}", e.what());
    }

    il2cpp_functions::Init();
    Logger.info("il2cpp_functions::Init() complete");

    try {
        bs_editor::hooks::InstallMenuHooks();
        Logger.info("InstallMenuHooks() complete");
    } catch (const std::exception& e) {
        Logger.error("InstallMenuHooks() threw: {}", e.what());
    } catch (...) {
        Logger.error("InstallMenuHooks() threw an unknown exception");
    }

    try {
        bs_editor::hooks::InstallGameplayHooks();
        Logger.info("InstallGameplayHooks() complete");
    } catch (const std::exception& e) {
        Logger.error("InstallGameplayHooks() threw: {}", e.what());
    } catch (...) {
        Logger.error("InstallGameplayHooks() threw an unknown exception");
    }

    Logger.info("bs-android-editor load() finished");
}
