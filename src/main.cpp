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
#include "bs-android-editor/Hooks/GameplayHooks.hpp"
#include "bs-android-editor/Hooks/MenuHooks.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "scotland2/shared/modloader.h"

extern "C" void setup(CModInfo* info) {
    info->id = MOD_ID;
    info->version = VERSION;
    info->version_long = 0;
    Logger.info("bs-android-editor setup complete");
}

extern "C" void load() {
    il2cpp_functions::Init();

    Logger.info("Installing bs-android-editor hooks...");
    bs_editor::hooks::InstallMenuHooks();
    bs_editor::hooks::InstallGameplayHooks();
    Logger.info("bs-android-editor hooks installed");
}
