#include "common/composition_mask.h"

namespace Raster {
    CompositionMask::CompositionMask(Json t_data) {
        this->compositionID = t_data["CompositionID"];
        this->op = static_cast<MaskOperation>(t_data["MaskOperation"].get<int>());
        this->precompose = t_data.contains("Precompose") ? t_data["Precompose"].get<bool>() : false;
    }

    Json CompositionMask::Serialize() {
        return {
            {"CompositionID", compositionID},
            {"MaskOperation", static_cast<int>(op)},
            {"Precompose", precompose}
        };
    }
};