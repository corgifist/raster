#include "attributes/attributes.h"
#include "float_attribute.h"
#include "vec4_attribute.h"
#include "common/randomizer.h"

namespace Raster {
    std::vector<AttributeDescription> Attributes::s_attributes;

    static AbstractAttribute SpawnFloatAttribute() {
        return (AbstractAttribute) std::make_unique<FloatAttribute>();
    }

    static AbstractAttribute SpawnVec4Attribute() {
        return (AbstractAttribute) std::make_unique<Vec4Attribute>();
    }

    std::optional<AttributeDescription> Attributes::GetAttributeDescriptionByPackageName(std::string t_packageName) {
        for (auto& entry : s_attributes) {
            if (entry.packageName == t_packageName) return entry;
        }
        return std::nullopt;
    }

    std::optional<AbstractAttribute> Attributes::InstantiateAttribute(std::string t_packageName) {
        auto description = GetAttributeDescriptionByPackageName(t_packageName);
        if (description.has_value()) {
            auto attribute = description.value().spawnProcedure();
            attribute->packageName = t_packageName;
            return attribute;
        }
        return std::nullopt;
    }

    std::optional<AbstractAttribute> Attributes::InstantiateSerializedAttribute(Json t_data) {
        if (t_data.contains("PackageName")) {
            auto attributeCandidate = InstantiateAttribute(t_data["PackageName"]);
            if (attributeCandidate.has_value()) {
                attributeCandidate.value()->id = t_data["ID"];
                attributeCandidate.value()->Load(t_data["Data"]);
                attributeCandidate.value()->packageName = t_data["PackageName"];
                return attributeCandidate;
            }
        }
        return std::nullopt;
    }

    std::optional<AbstractAttribute> Attributes::CopyAttribute(AbstractAttribute t_base) {
        auto copiedAttributeCandidate = Attributes::InstantiateSerializedAttribute(t_base->Serialize());
        if (copiedAttributeCandidate.has_value()) {
            auto& attribute = copiedAttributeCandidate.value();
            attribute->id = Randomizer::GetRandomInteger();
            for (auto& keyframe : attribute->keyframes) {
                keyframe.id = Randomizer::GetRandomInteger();
            }
            return attribute;
        }
        return std::nullopt;
    }

    void Attributes::Initialize() {
        s_attributes.push_back(AttributeDescription{
            .packageName = RASTER_PACKAGED_PACKAGE "float_attribute",
            .prettyName = ICON_FA_DIVIDE " Float",
            .spawnProcedure = SpawnFloatAttribute
        });

        s_attributes.push_back(AttributeDescription{
            .packageName = RASTER_PACKAGED_PACKAGE "vec4_attribute",
            .prettyName = ICON_FA_EXPAND " Vector4",
            .spawnProcedure = SpawnVec4Attribute
        });

        std::cout << "attributes initialization finished" << std::endl;
    }
};