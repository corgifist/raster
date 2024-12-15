#pragma once

#include "raster.h"

namespace Raster {
    struct Choice {
        std::vector<std::string> variants;
        int selectedVariant;

        Choice() : selectedVariant(0), variants() {}
        Choice(std::vector<std::string> t_variants) : selectedVariant(0), variants(t_variants) {}
        Choice(std::vector<std::string> t_variants, int t_selectedVariant) : selectedVariant(t_selectedVariant), variants(t_variants) {}
        Choice(Json t_data);

        Json Serialize();
    };
};