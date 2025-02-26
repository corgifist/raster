#pragma once

#include "common/plugin_base.h"
#include "common/libraries.h"

namespace Raster {
    struct Plugins {
        static std::vector<AbstractPlugin> s_plugins;

        static void Initialize();
        static void EarlyInitialize();
        static void WorkspaceInitialize();
        static void LateInitialize();
        static void SetupUI();
        static void WriteConfigs();

        static std::optional<AbstractPlugin> GetPluginByPackageName(std::string t_packageName);
        static std::optional<int> GetPluginIndexByPackageName(std::string t_packageName);
    };
};