#pragma once

#include "raster.h"
#include <glm/ext/matrix_transform.hpp>

namespace Raster {
    struct ConvolutionKernel {
        static std::vector<std::pair<std::string, ConvolutionKernel>> s_presets;

        float multiplier;
        glm::mat3 kernel;

        ConvolutionKernel() : multiplier(1.0), kernel(0, 0, 0, 0, 1, 0, 0, 0, 0) {}
        ConvolutionKernel(float t_multiplier, glm::mat3 t_kernel) : multiplier(t_multiplier), kernel(t_kernel) {}
        ConvolutionKernel(Json t_data);

        glm::mat3 GetKernel();

        Json Serialize();
    };
};