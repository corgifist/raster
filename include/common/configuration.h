#pragma once
#include "raster.h"
#include "typedefs.h"
#include "layout.h"

namespace Raster {
    struct Configuration {
    public:
        std::vector<Layout> layouts;
        int selectedLayout;

        Configuration(Json t_data);
        Configuration();

        Json& GetPluginData(std::string t_packageName);

        Json Serialize();
    private:
        Json m_pluginData;
    };
};