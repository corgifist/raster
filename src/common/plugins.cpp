#include "common/plugins.h"
#include "common/configuration.h"
#include "common/configuration.h"
#include "common/libraries.h"
#include "common/plugin_base.h"
#include "raster.h"
#include <filesystem>

namespace Raster {
    std::vector<AbstractPlugin> Plugins::s_plugins;

    void Plugins::Initialize() {
        auto pluginIterator = std::filesystem::recursive_directory_iterator("plugins");
        for (auto &entry : pluginIterator) {
            std::string transformedPath = std::regex_replace(
                GetBaseName(entry.path().string()), std::regex(".dll|.so|lib"), "");
            Libraries::LoadLibrary("plugins", transformedPath);
            auto pluginSpawn = Libraries::GetFunction<AbstractPlugin()>(transformedPath, "SpawnPlugin");
            try {
                s_plugins.push_back(pluginSpawn());
                print("instantiating plugin '" << transformedPath << "'");
            } catch (...) {
                print("failed to instantiate plugin '" << transformedPath << "'");
            }
        }
    }

    void Plugins::EarlyInitialize() {
        auto preferencesPluginCandidate = GetPluginByPackageName(RASTER_PACKAGED "preferences");
        if (preferencesPluginCandidate) {
            (*preferencesPluginCandidate)->OnEarlyInitialization();
        }
        for (auto& plugin : s_plugins) {
            if (plugin->PackageName() == RASTER_PACKAGED "preferences") continue;
            plugin->OnEarlyInitialization();
        }
    }

    void Plugins::WorkspaceInitialize() {
        auto preferencesPluginCandidate = GetPluginByPackageName(RASTER_PACKAGED "preferences");
        if (preferencesPluginCandidate) {
            (*preferencesPluginCandidate)->OnWorkspaceInitialization();
        }
        for (auto& plugin : s_plugins) {
            if (plugin->PackageName() == RASTER_PACKAGED "preferences") continue;
            plugin->OnWorkspaceInitialization();
        }
    }

    std::optional<AbstractPlugin> Plugins::GetPluginByPackageName(std::string t_packageName) {
        for (auto& plugin : s_plugins) {
            if (plugin->PackageName() == t_packageName) {
                return plugin;
            }
        }
        return std::nullopt;
    }

    std::optional<int> Plugins::GetPluginIndexByPackageName(std::string t_packageName) {
        int index = 0;
        for (auto& plugin : s_plugins) {
            if (plugin->PackageName() == t_packageName) {
                return index;
            }
            index++;
        }
        return std::nullopt;
    }

    void Plugins::WriteConfigs() {
        for (auto& plugin : s_plugins) {
            plugin->WriteConfigs();
        }
    }

};