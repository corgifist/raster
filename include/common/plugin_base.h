#pragma once

#include "raster.h"
#include "font/IconsFontAwesome5.h"

#define RASTER_SPAWN_PLUGIN(T) \
    RASTER_SPAWN_ABSTRACT(Raster::AbstractPlugin, T)

namespace Raster {
    struct PluginBase {
    public:
        PluginBase();

        std::string Name();
        std::string Description();
        std::string Icon();
        std::string PackageName();

        // this method is called before workspace initialization
        // primarily useful for loading various configs and localizations
        void OnEarlyInitialization();

        // this method is called right after workspace initialization
        // you're supposed to use this to add your own node implementations
        void OnWorkspaceInitialization();

        Json& GetPluginData();

        // should render ui in `Preferences` window
        void RenderProperties();

    private:    
        virtual std::string AbstractName() { return "Empty Plugin"; };
        virtual std::string AbstractDescription() { return "Description of Empty Plugin"; };
        virtual std::string AbstractIcon() { return ICON_FA_GEARS; }
        virtual std::string AbstractPackageName() { return "empty_plugin"; };
        virtual void AbstractRenderProperties() {};
        virtual void AbstractOnEarlyInitialization() {};
        virtual void AbstractOnWorkspaceInitialization() {};
    };

    using AbstractPlugin = std::shared_ptr<PluginBase>;
};