#pragma once
#include "raster.h"
#include "typedefs.h"

namespace Raster {
    struct Configuration {
        std::string localizationCode;

        Configuration(Json data);
        Configuration();

        Json Serialize();
    };
};