#pragma once

#include "raster.h"
#include "common/line2d.h"

namespace Raster {

    Line2D::Line2D(Json t_data) {
        this->begin = glm::vec2(t_data["Begin"][0], t_data["Begin"][1]);
        this->end = glm::vec2(t_data["End"][0], t_data["End"][1]);
        this->beginColor = t_data.contains("BeginColor") ? glm::vec4(t_data["BeginColor"][0], t_data["BeginColor"][1], t_data["BeginColor"][2], t_data["BeginColor"][3]) : glm::vec4(1);
        this->endColor = t_data.contains("EndColor") ? glm::vec4(t_data["EndColor"][0], t_data["EndColor"][1], t_data["EndColor"][2], t_data["EndColor"][3]) : glm::vec4(1);
    }

    Json Line2D::Serialize() {
        return {
            {"Begin", {begin.x, begin.y}},
            {"End", {end.x, end.y}},
            {"BeginColor", {beginColor.r, beginColor.g, beginColor.b, beginColor.a}},
            {"EndColor", {endColor.r, endColor.g, endColor.b, endColor.a}}
        };
    }
};