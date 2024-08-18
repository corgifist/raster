#include "common/easing_base.h"

namespace Raster {
    void EasingBase::Initialize() {
        this->id = Randomizer::GetRandomInteger();
        this->prettyName = "Easing";
    }

    void EasingBase::RenderDetails() {
        AbstractRenderDetails();
    }

    Json EasingBase::Serialize() {
        return {
            {"ID", id},
            {"PackageName", packageName},
            {"Data", AbstractSerialize()}
        };
    }

    void EasingBase::Load(Json t_data) {
        AbstractLoad(t_data);
    }
};