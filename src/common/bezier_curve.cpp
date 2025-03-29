#include "common/bezier_curve.h"
#include "common/thread_unique_value.h"
#include "raster.h"
#include <glm/common.hpp>

namespace Raster {
    BezierCurve::BezierCurve() {
        this->points.push_back({-1, -1});
        this->points.push_back({1, 1});
        this->smoothCurve = false;
    }

    BezierCurve::BezierCurve(Json t_data) {
        for (auto& point : t_data["Points"]) {
            points.push_back({point[0], point[1]});
        }
        if (t_data.contains("SmoothCurve")) {
            this->smoothCurve = t_data["SmoothCurve"];
        }
    }

    static glm::vec2 InternalGet(float t_percentage, std::vector<glm::vec2> points) {
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

    std::pair<double, double> catmullRom(const std::pair<double, double>& p0,
                const std::pair<double, double>& p1,
                const std::pair<double, double>& p2,
                const std::pair<double, double>& p3,
                double t) {
        double t2 = t * t;
        double t3 = t2 * t;

        double x = 0.5 * ((2.0 * p1.first) +
        (-p0.first + p2.first) * t +
        (2.0 * p0.first - 5.0 * p1.first + 4.0 * p2.first - p3.first) * t2 +
        (-p0.first + 3.0 * p1.first - 3.0 * p2.first + p3.first) * t3);

        double y = 0.5 * ((2.0 * p1.second) +
        (-p0.second + p2.second) * t +
        (2.0 * p0.second - 5.0 * p1.second + 4.0 * p2.second - p3.second) * t2 +
        (-p0.second + 3.0 * p1.second - 3.0 * p2.second + p3.second) * t3);

        return {x, y};
        }

    glm::vec2 BezierCurve::Get(float t_percentage) {
        if (smoothCurve) {
            int p0, p1, p2, p3;
            auto t = t_percentage * (points.size() - 1);
            p1 = static_cast<int>(t);
            p0 = (p1 > 0) ? p1 - 1 : p1;
            p2 = (p1 + 1 < points.size()) ? p1 + 1 : p1;
            p3 = (p1 + 2 < points.size()) ? p1 + 2 : p2;
    
            t = t - p1; // Normalize t to [0, 1]
    
            auto p = catmullRom({points[p0].x, points[p0].y}, {points[p1].x, points[p1].y}, {points[p2].x, points[p2].y}, {points[p3].x, points[p3].y}, t);
            return {p.first, p.second};
        }
        return InternalGet(t_percentage, points);
    }

    Json BezierCurve::Serialize() {
        Json result = Json::object();
        result["Points"] = Json::array();
        for (auto& point : points) {
            result["Points"].push_back(Json::array({point.x, point.y}));
        }
        result["SmoothCurve"] = smoothCurve;
        return result;
    }
};