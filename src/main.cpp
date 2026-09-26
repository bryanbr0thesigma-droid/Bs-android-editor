// Mod entry point for the Scotland2 modloader + beatsaber-hook 8.x.
//
// Verified directly against QuestPackageManager/beatsaber-hook@v8.2.1 and
// sc2ad/scotland2: setup(CModInfo*)/load() match modloader::SetupFunc and
// modloader::LoadFunc exactly (see shared/loader.hpp), and beatsaber-hook's
// own README confirms i2c::functions::initialize() (not the old
// il2cpp_functions::Init()) is what needs to run once il2cpp is up.
#include "bs-android-editor/main.hpp"
#include "bs-android-editor/Hooks/GameplayHooks.hpp"
#include "bs-android-editor/Hooks/MenuHooks.hpp"

#include "beatsaber-hook/shared/api.hpp"
#include "scotland2/shared/modloader.h"

extern "C" void setup(CModInfo* info) {
    info->id = MOD_ID;
    info->version = VERSION;
    info->version_long = 0;
    Logger.info("bs-android-editor setup complete");
}

extern "C" void load() {
    i2c::functions::initialize();

    Logger.info("Installing bs-android-editor hooks...");
    bs_editor::hooks::InstallMenuHooks();
    bs_editor::hooks::InstallGameplayHooks();
    Logger.info("bs-android-editor hooks installed");
}
