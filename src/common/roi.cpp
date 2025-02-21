#include "common/roi.h"

namespace Raster {
    ROI::ROI(Json t_data) {
        this->upperLeft = glm::vec2(t_data["UpperLeft"][0].get<float>(), t_data["UpperLeft"][1].get<float>());
        this->bottomRight = glm::vec2(t_data["BottomRight"][0].get<float>(), t_data["BottomRight"][1].get<float>());
    }

    Json ROI::Serialize() {
        return {
            {"UpperLeft", {upperLeft[0], upperLeft[1]}},
            {"BottomRight", {bottomRight[0], bottomRight[1]}}
        };
    }
};