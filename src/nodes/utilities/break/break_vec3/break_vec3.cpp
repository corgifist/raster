#include "break_vec3.h"

namespace Raster {

    BreakVec3::BreakVec3() {
        NodeBase::Initialize();

        SetupAttribute("Vector", glm::vec3());

        AddOutputPin("X");
        AddOutputPin("Y");
        AddOutputPin("Z");
    }

    AbstractPinMap BreakVec3::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto vectorCandidate = GetAttribute<glm::vec3>("Vector");
        if (vectorCandidate.has_value()) {
            auto& vector = vectorCandidate.value();
            TryAppendAbstractPinMap(result, "X", vector.x);
            TryAppendAbstractPinMap(result, "Y", vector.y);
            TryAppendAbstractPinMap(result, "Z", vector.z);
        }
        return result;
    }

    void BreakVec3::AbstractRenderProperties() {
        RenderAttributeProperty("Vector");
    }
    
    void BreakVec3::AbstractLoadSerialized(Json t_data) {
        SetAttributeValue("Vector", glm::vec3{t_data["Vector"][0].get<float>(), t_data["Vector"][1].get<float>(), t_data["Vector"][2].get<float>()});
    }

    Json BreakVec3::AbstractSerialize() {
        auto vec = RASTER_ATTRIBUTE_CAST(glm::vec3, "Vector");
        return {
            {"Vector", {vec.x, vec.y, vec.z}}
        };
    }

    bool BreakVec3::AbstractDetailsAvailable() {
        return false;
    }

    std::string BreakVec3::AbstractHeader() {
        return "Break Vec3";
    }

    std::string BreakVec3::Icon() {
        return ICON_FA_EXPAND;
    }

    std::optional<std::string> BreakVec3::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::BreakVec3>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Break Vec3",
            .packageName = RASTER_PACKAGED "break_vec3",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}