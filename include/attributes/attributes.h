#pragma once

#include "raster.h"
#include "common/typedefs.h"
#include "common/attribute.h"

namespace Raster {
        
    struct AttributeDescription {
        std::string packageName;
        std::string prettyName;
        AttributeSpawnProcedure spawnProcedure;
    };

    struct Attributes {
        static std::vector<AttributeDescription> s_attributes;

        static std::optional<AttributeDescription> GetAttributeDescriptionByPackageName(std::string t_packageName);
        static std::optional<AbstractAttribute> InstantiateAttribute(std::string t_packageName);
        static std::optional<AbstractAttribute> InstantiateSerializedAttribute(Json t_data);

        static void Initialize();
    };
};