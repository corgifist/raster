#pragma once

#include "raster.h"

namespace Raster {
    struct ZIP {
        static void Extract(std::string t_zipFile, std::string t_outputDirectory);
        static void Pack(std::string t_zipFile, std::string t_inputDirectory);
    };
};