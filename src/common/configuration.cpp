#include "common/common.h"

namespace Raster {
    Configuration::Configuration() {
        this->localizationCode = "en";
    }

    Configuration::Configuration(Json data) {
        this->localizationCode = data["Localization"];
    }

    Json Configuration::Serialize() {
        return {
            {"Localization", this->localizationCode}
        };
    }
};