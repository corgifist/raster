#include "common/attribute_metadata.h"

namespace Raster {
    SliderRangeMetadata::SliderRangeMetadata(float t_min, float t_max) {
        this->min = t_min;
        this->max = t_max;
    }

    SliderStepMetadata::SliderStepMetadata(float t_step) {
        this->step = t_step;
    }

    FormatStringMetadata::FormatStringMetadata(std::string t_format) {
        this->format = t_format;
    }

    SliderBaseMetadata::SliderBaseMetadata(float t_base) {
        this->base = t_base;
    }

    IconMetadata::IconMetadata(std::string t_icon) {
        this->icon = t_icon;
    }
};