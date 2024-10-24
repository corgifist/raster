#pragma once

#include "raster.h"

namespace Raster {
    struct SliderRangeMetadata {
        float min, max;
        SliderRangeMetadata(float t_min, float t_max);
    };

    struct FormatStringMetadata {
        std::string format;
        FormatStringMetadata(std::string t_format);
    };

    struct SliderStepMetadata {
        float step;
        SliderStepMetadata(float t_step);
    };

    // Name can be a little confusing, but this metadata object simply
    // makes 1.0f to be displayed as 100 (if base == 100) in sliders.
    // This setting does nothing if attribute is integer.
    struct SliderBaseMetadata {
        float base;
        SliderBaseMetadata(float t_step);
    };

    // TODO: this must make vector4 appear as a color4
    struct Vec4ColorPickerMetadata {

    };  

    // Replaces standard ICON_FA_LIST icon with the specified icon in the `Node Properties` window
    struct IconMetadata {
        std::string icon;

        IconMetadata(std::string t_icon);
    };
};