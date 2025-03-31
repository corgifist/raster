#include "common/color_management.h"
#include "raster.h"
#include <OpenColorIO/OpenColorIO.h>
#include <OpenColorIO/OpenColorTypes.h>
#include <string>

namespace Raster {
    OCIO::ConstConfigRcPtr ColorManagement::s_config;
    std::string ColorManagement::s_display;
    std::string ColorManagement::s_look;
    std::string ColorManagement::s_transformName;
    bool ColorManagement::s_useLegacyGPU = false;

    void ColorManagement::Initialize() {
        s_config = OCIO::GetCurrentConfig();
        s_config->getDefaultDisplay();
        s_display = s_config->getDefaultDisplay();
        s_transformName = s_config->getDefaultView(s_display.c_str());
        s_look = s_config->getDisplayViewLooks(s_display.c_str(), s_transformName.c_str());
        DUMP_VAR(s_display);
        DUMP_VAR(s_transformName);
        DUMP_VAR(s_look); 
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