#include "preferences.h"
#include "common/configuration.h"
#include "common/localization.h"
#include "common/plugin_base.h"
#include "common/randomizer.h"
#include "font/IconsFontAwesome5.h"
#include "common/workspace.h"
#include "raster.h"
#include <filesystem>
#include "common/ui_helpers.h"

namespace Raster {

    static Json CreateLayout(std::string t_name) {
        return {
            {"ID", Randomizer::GetRandomInteger()},
            {"Name", t_name}
        };
    }

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

        if (!std::filesystem::exists(internalRasterFolder + "layouts/")) {
            std::filesystem::create_directory(internalRasterFolder + "layouts/");
        }

        auto savedConfig = ReadJson(internalRasterFolder + "config.json");
        auto defaultConfiguration = GetDefaultConfiguration();
        if (!savedConfig["PluginData"].contains(RASTER_PACKAGED "preferences")) {
            savedConfig["PluginData"][RASTER_PACKAGED "preferences"] = Json::object();
        }
        for (auto& defaultItem : defaultConfiguration.items()) {
            if (!savedConfig["PluginData"][RASTER_PACKAGED "preferences"].contains(defaultItem.key())) {
                savedConfig["PluginData"][RASTER_PACKAGED "preferences"][defaultItem.key()] = defaultItem.value();
            }
        }

        Workspace::s_configuration = Configuration(savedConfig);

        auto& pluginData = GetPluginData();
        if (pluginData.empty()) {
            pluginData = GetDefaultConfiguration();
        }
        if (pluginData["Layouts"].empty()) {
            pluginData["Layouts"] = Json::array();
            pluginData["Layouts"].push_back(CreateLayout("Main Layout"));
            pluginData["SelectedLayout"] = pluginData["Layouts"][0]["ID"];
        }
        std::string localizationCode = pluginData["LocalizationCode"];
        try {
            DUMP_VAR(localizationCode);
            Localization::Load(ReadJson(FormatString("localizations/%s.json", localizationCode.c_str())));
        } catch (...) {
            RASTER_LOG("failed to load localization '" << localizationCode << "'");
        }
    }

    void PreferencesPlugin::AbstractWriteConfigs() {
        auto dump = Workspace::s_configuration.Serialize().dump();
        WriteFile(GetHomePath() + "/.raster/config.json", dump);
    }

    void PreferencesPlugin::AbstractRenderProperties() {
        auto& pluginData = GetPluginData();
        
        UIHelpers::RenderJsonEditor(pluginData);

        if (UIHelpers::CenteredButton(FormatString("%s %s", ICON_FA_RECYCLE, Localization::GetString("RESET_TO_DEFAULTS").c_str()).c_str())) {
            pluginData = GetDefaultConfiguration();
        }
    }

    Json PreferencesPlugin::GetDefaultConfiguration() {
        return {
            {"LocalizationCode", "en"},
            {"LocalizationSearchPaths", {"."}},
            {"Layouts", Json::array()},
            {"SelectedLayout", -1}
        };
    }
};

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractPlugin SpawnPlugin() {
        return RASTER_SPAWN_PLUGIN(Raster::PreferencesPlugin);
    }
}