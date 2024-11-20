#pragma once

#include "raster.h"
#include "randomizer.h"

namespace Raster {

    #pragma pack(push, 1)
    struct RawGradientStop1D {
        float percentage;
        glm::vec4 color;

        RawGradientStop1D() : percentage(0.0f), color(glm::vec4(0)) {}
        RawGradientStop1D(float t_percentage, glm::vec4 t_color) : percentage(t_percentage), color(t_color) {}
    };
    #pragma pack(pop)

    struct GradientStop1D {
        int id;
        float percentage;
        glm::vec4 color;

        GradientStop1D(float t_percentage, glm::vec4 t_color) : percentage(t_percentage), color(t_color) {
            this->id = Randomizer::GetRandomInteger();
        }
        GradientStop1D() : percentage(0), color(glm::vec4(0)) {
            this->id = Randomizer::GetRandomInteger();
        }
    };

    struct Gradient1D {
        std::vector<GradientStop1D> stops;

        Gradient1D();
        Gradient1D(Json t_data);

        glm::vec4 Get(float t_percentage);

        void AddStop(float t_percentage, glm::vec4 t_color);
        void SortStops();

        void FillToBuffer(char* t_buffer);

        std::vector<RawGradientStop1D> GetRawStops();

        Gradient1D MatchStopsCount(Gradient1D& t_reference);

        Json Serialize();
    };
};