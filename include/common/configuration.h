#pragma once
#include "raster.h"
#include "typedefs.h"

namespace Raster {
    struct Configuration {
    public:
        Configuration(Json t_data);
        Configuration();

        Json& GetPluginData(std::string t_packageName);

        Json Serialize();
    private:
        Json m_pluginData;
    };
};