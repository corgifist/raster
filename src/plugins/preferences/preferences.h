#pragma once

#include "raster.h"
#include "common/plugin_base.h"

namespace Raster {
    struct PreferencesPlugin : public PluginBase {
        PreferencesPlugin() : PluginBase() {}

        std::string AbstractName();
        std::string AbstractDescription();
        std::string AbstractIcon();
        std::string AbstractPackageName();

        void AbstractOnEarlyInitialization();
        void AbstractOnLateInitialization();

        void AbstractRenderProperties();

        void AbstractWriteConfigs();

        static Json GetDefaultConfiguration();
    };
};