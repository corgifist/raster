#include "common/color_management.h"
#include "raster.h"
#include <OpenColorIO/OpenColorIO.h>
#include <OpenColorIO/OpenColorTypes.h>
#include <cstdlib>
#include <string>

namespace Raster {
    OCIO::ConstConfigRcPtr ColorManagement::s_config;
    std::string ColorManagement::s_display;
    std::string ColorManagement::s_look;
    std::string ColorManagement::s_transformName;
    bool ColorManagement::s_useLegacyGPU = false;
    std::vector<std::string> ColorManagement::s_colorspaces;
    std::string ColorManagement::s_defaultColorspace;

    void ColorManagement::Initialize() {
        setenv("OCIO", "ocioconf/config.ocio", 0);
        s_config = OCIO::Config::CreateFromEnv();
        s_config->getDefaultDisplay();
        s_display = s_config->getDefaultDisplay();
        s_transformName = s_config->getDefaultView(s_display.c_str());
        s_look = s_config->getDisplayViewLooks(s_display.c_str(), s_transformName.c_str());
        DUMP_VAR(s_display);
        DUMP_VAR(s_transformName);
        DUMP_VAR(s_look); 
        RASTER_LOG("available OCIO colorspaces:");
        for (int i = 0; i < s_config->getNumColorSpaces(); i++) {
            print("\t" << s_config->getColorSpaceNameByIndex(i));
            s_colorspaces.push_back(s_config->getColorSpaceNameByIndex(i));
        }
        s_defaultColorspace = "Raw";
    }

    std::string ColorManagement::GetColorSpaceFromFile(std::string t_path) {
        try {
            std::string candidate = s_config->getColorSpaceFromFilepath(t_path.c_str());
            if (!candidate.empty()) return candidate;
            return OCIO::ROLE_SCENE_LINEAR;
        } catch (...) {
            return "";
        }
    }
};