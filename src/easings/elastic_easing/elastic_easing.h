#pragma once

#include "common/easing_base.h"
#include "common/easings.h"

namespace Raster {
    struct ElasticEasing : public EasingBase {
    public:
        ElasticEasing();

        float BaseGet(float t_percentage);
        float Get(float t_percentage);

        void AbstractRenderDetails();

        void AbstractLoad(Json t_data);
        Json AbstractSerialize();

    private:
        float m_amplitude;
        float m_speed;
    };
};