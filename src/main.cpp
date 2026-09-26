// Mod entry point. Targets the Scotland2 modloader (the current QuestLoader
// successor) + beatsaber-hook 6.x's setup()/load() convention. If your qpm
// dependencies resolve to a different modloader (plain QuestLoader) or an
// older beatsaber-hook, check that project's example mod for the current
// exact entry-point signatures — this shape has changed a few times across
// the ecosystem's history.
#include "bs-android-editor/main.hpp"
#include "bs-android-editor/Hooks/GameplayHooks.hpp"
#include "bs-android-editor/Hooks/MenuHooks.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "scotland2/shared/loader.hpp"

#include <memory>

namespace {
std::unique_ptr<Logger> g_logger;
}

Logger& getLogger() {
    static Logger& instance = *(g_logger = std::make_unique<Logger>(ModInfo{MOD_ID, VERSION, 0}));
    return instance;
}

extern "C" void setup(CModInfo* info) {
    info->id = MOD_ID;
    info->version = VERSION;
    info->version_long = 0;
    getLogger().info("bs-android-editor setup complete");
}

extern "C" void load() {
    il2cpp_functions::Init();

    getLogger().info("Installing bs-android-editor hooks...");
    bs_editor::hooks::InstallMenuHooks();
    bs_editor::hooks::InstallGameplayHooks();
    getLogger().info("bs-android-editor hooks installed");
}
