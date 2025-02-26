#pragma once

#include "raster.h"
#include "common/plugin_base.h"

namespace Raster {
    struct RenderingPlugin : public PluginBase {
        RenderingPlugin() : PluginBase() {}

        std::string AbstractName();
        std::string AbstractDescription();
        std::string AbstractIcon();
        std::string AbstractPackageName();

        void AbstractOnEarlyInitialization();
        void AbstractRenderProperties();

        static Json GetDefaultConfiguration();
    };
};