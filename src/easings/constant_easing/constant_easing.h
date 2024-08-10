#pragma once

#include "common/easing_base.h"
#include "common/easings.h"

#define EPSILON 1e-6

namespace Raster {
    struct ConstantEasing : public EasingBase {
    public:
        ConstantEasing();

        float Get(float t_percentage);

        void AbstractRenderDetails();

        void AbstractLoad(Json t_data);
        Json AbstractSerialize();

    private:
        float m_constant;
    };
};