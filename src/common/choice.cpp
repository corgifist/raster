#include "common/choice.h"

namespace Raster {
    Choice::Choice(Json t_data) {
        if (t_data.contains("Variants")) {
            this->variants = t_data["Variants"];
        }
        if (t_data.contains("SelectedVariant")) {
            this->selectedVariant = t_data["SelectedVariant"];
        }
    }

    Json Choice::Serialize() {
        return {
            {"Variants", variants},
            {"SelectedVariant", selectedVariant}
        };
    }
};