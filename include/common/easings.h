#pragma once

#include "easing_base.h"
#include "libraries.h"

namespace Raster {
    struct EasingDescription {
        std::string prettyName;
        std::string packageName;
    };

    struct EasingImplementation {
        EasingDescription description;
        EasingSpawnProcedure spawn;
    };

    struct Easings {
        static std::vector<EasingImplementation> s_implementations;

        static void Initialize();
        static std::optional<AbstractEasing> InstantiateEasing(std::string t_packageName);
        static std::optional<AbstractEasing> InstantiateSerializedEasing(Json t_data);

        static std::optional<EasingImplementation> GetEasingImplementationByPackageName(std::string t_packageName);
    };
}