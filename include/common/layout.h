#pragma once

#include "raster.h"
#include "common/typedefs.h"
#include "user_interface.h"

namespace Raster {
    struct Layout {
        int id;
        std::string name;
        std::vector<AbstractUserInterface> windows;

        Layout();
        Layout(std::string t_name);
        Layout(Json t_data);

        Json Serialize();
    };
};