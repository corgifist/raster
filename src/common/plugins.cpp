#include "common/plugins.h"
#include "common/configuration.h"
#include "common/configuration.h"
#include "common/libraries.h"
#include "common/plugin_base.h"
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
        for (auto& plugin : s_plugins) {
            plugin->OnEarlyInitialization();
        }
    }

    void Plugins::WorkspaceInitialize() {
        for (auto& plugin : s_plugins) {
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
};