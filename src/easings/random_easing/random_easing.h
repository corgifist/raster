#pragma once

#include "common/easing_base.h"
#include "common/easings.h"

namespace Raster {
    struct RandomEasing : public EasingBase {
    public:
        RandomEasing();

        float Get(float t_percentage);

        void AbstractRenderDetails();

        void AbstractLoad(Json t_data);
        Json AbstractSerialize();

    private:
        float m_seed1;
        float m_seed2;
        float m_seed3;
        float m_amplitude;
    };
};