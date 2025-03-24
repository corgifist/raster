#include "common/convolution_kernel.h"

namespace Raster {

    std::vector<std::pair<std::string, ConvolutionKernel>> ConvolutionKernel::s_presets = {
        {"Identity", {1.0f, {0, 0, 0, 0, 1, 0, 0, 0, 0}}},
        {"Edge Detection (Horizontal)", {1.0f, {-1, -1, -1, 0, 0, 0, 1, 1, 1}}},
        {"Edge Detection (Vertical)", {1.0f, {-1, 0, 1, -1, 0, 1, -1, 0, 1}}},
        {"Sharpen", {1.0f, {0, -1, 0 , -1, 5, -1, 0, -1, 0}}},
        {"Box Blur", {1.0f, {1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f}}},
        {"Gaussian Blur", {1.0f / 16.0f, {1, 2, 1, 2, 4, 2, 1, 2, 1}}},
        {"Sobel (Horizontal)", {1.0f, {-1, 0, 1, -2, 0, 2, -1, 0, 1}}},
        {"Sobel (Vertical)", {1.0f, {-1, -2 , -1, 0, 0, 0, 1, 2, 1}}},
        {"Prewitt (Horizontal)", {1.0f, {-1, 0, 1, -1, 0, 1, -1, 0, 1}}},
        {"Prewitt (Vertical)", {1.0f, {-1, -1, -1, 0, 0, 0, 1, 1, 1}}},
        {"Laplacian", {1.0f, {0, -1, 0, -1, 4, -1, 0, -1, 0}}},
        {"Emboss", {1.0f, {-2, -1, 0, -1, 1, 1, 0, 1, 2}}}
    };

    ConvolutionKernel::ConvolutionKernel(Json t_data) {
        this->kernel = glm::mat3(t_data["Kernel"][0], t_data["Kernel"][1], t_data["Kernel"][2], t_data["Kernel"][3], t_data["Kernel"][4], t_data["Kernel"][5], t_data["Kernel"][6], t_data["Kernel"][7], t_data["Kernel"][8]);
        this->multiplier = t_data["Multiplier"];
    }

    glm::mat3 ConvolutionKernel::GetKernel() {
        return kernel * multiplier;
    }

    Json ConvolutionKernel::Serialize() {
        return {
            {"Multiplier", multiplier},
            {"Kernel", {kernel[0][0], kernel[0][1], kernel[0][2], kernel[1][0], kernel[1][1], kernel[1][2], kernel[2][0], kernel[2][1], kernel[2][2]}}
        };
    }
}