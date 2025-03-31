#pragma once

#include "raster.h"
#include <OpenColorIO/OpenColorIO.h>
#include <OpenColorIO/OpenColorTypes.h>
namespace OCIO = OCIO_NAMESPACE;

namespace Raster {
    struct ColorManagement {
        static OCIO::ConstConfigRcPtr s_config;
        static std::string s_display;
        static std::string s_transformName;
        static std::string s_look;
        static bool s_useLegacyGPU;

        static void Initialize();
        static std::string GetColorSpaceFromFile(std::string t_path);
    };
};