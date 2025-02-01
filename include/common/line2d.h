#pragma once

#include "raster.h"
#include "common/typedefs.h"

namespace Raster {  
    struct Line2D {
        glm::vec2 begin, end;

        Line2D() : begin(glm::vec2(-1)), end(glm::vec2(1)) {}
        Line2D(glm::vec2 t_begin, glm::vec2 t_end) : begin(t_begin), end(t_end) {}
        Line2D(Json t_data);

        Json Serialize();
    };
};