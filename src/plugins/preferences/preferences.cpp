#include "preferences.h"
#include "common/configuration.h"
#include "common/plugin_base.h"
#include "font/IconsFontAwesome5.h"
#include "common/workspace.h"
#include "raster.h"
#include <filesystem>

namespace Raster {
    std::string PreferencesPlugin::AbstractName() {
        return "Preferences";
    }

    std::string PreferencesPlugin::AbstractDescription() {
        return "Performs startup initialization of Raster";
    }

    std::string PreferencesPlugin::AbstractIcon() {
        return ICON_FA_SCREWDRIVER;
    }

    std::string PreferencesPlugin::AbstractPackageName() {
        return RASTER_PACKAGED "preferences";
    }

    void PreferencesPlugin::AbstractOnEarlyInitialization() {
        auto internalRasterFolder = GetHomePath() + "/.raster/";
        if (!std::filesystem::exists(internalRasterFolder)) {
            RASTER_LOG("creating '" << internalRasterFolder << "' folder");
            std::filesystem::create_directory(internalRasterFolder);
        }

        if (!std::filesystem::exists(internalRasterFolder + "config.json")) {
            WriteFile(internalRasterFolder + "config.json", Configuration().Serialize().dump());
        }

        Workspace::s_configuration = Configuration(ReadJson(internalRasterFolder + "config.json"));

        auto& pluginData = GetPluginData();
        if (pluginData.empty()) {
            pluginData = GetDefaultConfiguration();
        }
        std::string localizationCode = pluginData["LocalizationCode"];
        try {
            DUMP_VAR(localizationCode);
            Localization::Load(ReadJson(FormatString("misc/localizations/%s.json", localizationCode.c_str())));
        } catch (...) {
            RASTER_LOG("failed to load localization '" << localizationCode << "'");
        }
    }

    Json PreferencesPlugin::GetDefaultConfiguration() {
        return {
            {"LocalizationCode", "en"}
        };
    }
};

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractPlugin SpawnPlugin() {
        return RASTER_SPAWN_PLUGIN(Raster::PreferencesPlugin);
    }
}