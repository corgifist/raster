#pragma once

#include "raster.h"
#include "common/typedefs.h"
#include "common/attribute.h"

namespace Raster {
    struct AttributeImplementation {
        AttributeDescription description;
        AttributeSpawnProcedure spawn;
    };

    struct Attributes {
        static std::vector<AttributeImplementation> s_implementations;

        static std::optional<AttributeImplementation> GetAttributeImplementationByPackageName(std::string t_packageName);
        static std::optional<AbstractAttribute> InstantiateAttribute(std::string t_packageName);
        static std::optional<AbstractAttribute> InstantiateSerializedAttribute(Json t_data);

        static std::optional<AbstractAttribute> CopyAttribute(AbstractAttribute t_base);

        static void Initialize();
    };
};