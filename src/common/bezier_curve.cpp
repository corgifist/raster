#include "common/bezier_curve.h"
#include "common/thread_unique_value.h"

namespace Raster {
    BezierCurve::BezierCurve() {
        this->points.push_back({-1, -1});
        this->points.push_back({1, 1});
    }

    BezierCurve::BezierCurve(Json t_data) {
        for (auto& point : t_data) {
            points.push_back({point[0], point[1]});
        }
    }

    glm::vec2 BezierCurve::Get(float t_percentage) {
        static ThreadUniqueValue<std::vector<glm::vec2>> s_temporaryStorage;
        auto& temporaryStorage = s_temporaryStorage.Get();
        if (temporaryStorage.size() < points.size()) {
            temporaryStorage.resize(points.size());
        }
        memcpy(temporaryStorage.data(), points.data(), points.size() * sizeof(glm::vec2));
        int i = points.size() - 1;
        while (i > 0) {
            for (int k = 0; k < i; k++)
                temporaryStorage[k] = temporaryStorage[k] + t_percentage * ( temporaryStorage[k+1] - temporaryStorage[k] );
            i--;
        }
        return temporaryStorage[0];
    }

    Json BezierCurve::Serialize() {
        Json result = Json::array();
        for (auto& point : points) {
            result.push_back(Json::array({point.x, point.y}));
        }
        return result;
    }
};