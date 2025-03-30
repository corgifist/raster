#pragma once

#include "raster.h"

namespace Raster {

    struct Example {
        std::string name;
        std::string path;
        std::string icon;

        Example(Json t_data);
    };

    struct Examples {
        static std::vector<Example> s_examples;

        static void Initialize();
    };
};