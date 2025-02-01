#pragma once

#include "raster.h"
#include "common/line2d.h"

namespace Raster {

    Line2D::Line2D(Json t_data) {
        this->begin = glm::vec2(t_data["Begin"][0], t_data["Begin"][1]);
        this->end = glm::vec2(t_data["End"][0], t_data["End"][1]);
    }

    Json Line2D::Serialize() {
        return {
            {"Begin", {begin.x, begin.y}},
            {"End", {end.x, end.y}}
        };
    }
};