#pragma once

#include "common/plugin_base.h"

namespace Raster {
    struct MatchboxEffectsPlugin : public PluginBase {
        MatchboxEffectsPlugin() : PluginBase() {}

        std::string AbstractName();
        std::string AbstractDescription();
        std::string AbstractIcon();
        std::string AbstractPackageName();

        void AbstractOnWorkspaceInitialization();
        void LoadMatchboxEffects();
    };
};