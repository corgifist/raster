#pragma once

#include "raster.h"
#include "common/typedefs.h"

namespace Raster {  
    struct Line2D {
        glm::vec2 begin, end;
        glm::vec4 beginColor, endColor;

        Line2D() : begin(glm::vec2(-1)), end(glm::vec2(1)), beginColor(glm::vec4(1)), endColor(glm::vec4(1)) {}
        Line2D(glm::vec2 t_begin, glm::vec2 t_end) : begin(t_begin), end(t_end), beginColor(glm::vec4(1)), endColor(glm::vec4(1)) {}
        Line2D(glm::vec2 t_begin, glm::vec2 t_end, glm::vec4 t_beginColor, glm::vec4 t_endColor) : begin(t_begin), end(t_end), beginColor(t_beginColor), endColor(t_endColor) {}
        Line2D(Json t_data);

        Json Serialize();
    };
};