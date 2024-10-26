#pragma once

#include "raster.h"

namespace Raster {
    // assets can be identified by their id
    // cause id is just an integer, we need an ability to distinguish between ID and non-ID integers
    // this helper class does exactly what we need!
    struct AssetID {
        int id;

        AssetID() : id(0) {}

        AssetID(int t_id) : id(t_id) {}
    };
};