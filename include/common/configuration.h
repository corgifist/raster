#pragma once
#include "raster.h"
#include "typedefs.h"
#include "layout.h"

namespace Raster {
    struct Configuration {
    public:
        std::vector<Layout> layouts;
        int selectedLayout;


        // [[Project Name, Project Path], [Project Name, Project Path], ...]
        std::vector<std::vector<std::string>> recentProjects;
        std::string lastProjectPath;
        std::string lastProjectName;

        Configuration(Json t_data);
        Configuration();

        Json& GetPluginData(std::string t_packageName);

        Json Serialize();
    private:
        Json m_pluginData;
    };
};