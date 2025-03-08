#pragma once

#include "common/easing_base.h"
#include "common/easings.h"

namespace Raster {
    struct LinearEasing : public EasingBase {
    public:
        LinearEasing();

        float Get(float t_percentage);

        void AbstractRenderDetails();

        void AbstractLoad(Json t_data);
        Json AbstractSerialize();

    private:
        float m_slope;
        float m_flip;
        float m_shift;
    };
};