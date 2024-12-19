#pragma once

#include "common/plugin_base.h"

namespace Raster {
    struct XMLEffectsPlugin : public PluginBase {
        XMLEffectsPlugin() : PluginBase() {}

        std::string AbstractName();
        std::string AbstractDescription();
        std::string AbstractIcon();
        std::string AbstractPackageName();

        void AbstractOnWorkspaceInitialization();
    };
};