#pragma once

#include "common/easing_base.h"
#include "common/easings.h"

#define EPSILON 1e-6

namespace Raster {

    struct CurvePreset {
        std::string name;
        std::string category;
        std::string filteredCategoryName;
        std::string filteredName;
        glm::vec4 points;

        CurvePreset(std::string t_name, glm::vec4 t_points);
    };

    struct BezierEasing : public EasingBase {
    public:
        BezierEasing();

        float Get(float t_percentage);

        void AbstractRenderDetails();

        void AbstractLoad(Json t_data);
        Json AbstractSerialize();

    private:

        float SampleCurveX(double t);
        float SampleCurveY(double t);
        float SampleCurveDerivativeX(double t);

        float SolveCurveX(double x, double epsilon);
        float Solve(double x, double epsilon);

        float m_cx, m_bx, m_ax, m_cy, m_by, m_ay;

        glm::vec4 m_points;
        bool m_constrained;
    };
};