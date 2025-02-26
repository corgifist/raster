#pragma once

#include "raster.h"
#include "common/plugin_base.h"

namespace Raster {
    struct UIPlugin : public PluginBase {
        UIPlugin() : PluginBase() {}

        std::string AbstractName();
        std::string AbstractDescription();
        std::string AbstractIcon();
        std::string AbstractPackageName();

        void AbstractSetupUI();
        
        void AbstractRenderProperties();
    };
};