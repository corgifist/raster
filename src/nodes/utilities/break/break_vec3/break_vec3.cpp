#include "break_vec3.h"

namespace Raster {

    BreakVec3::BreakVec3() {
        NodeBase::Initialize();

        SetupAttribute("Vector", glm::vec3());

        AddOutputPin("X");
        AddOutputPin("Y");
        AddOutputPin("Z");
    }

    AbstractPinMap BreakVec3::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto vectorCandidate = GetAttribute<glm::vec3>("Vector", t_contextData);
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
        DeserializeAllAttributes(t_data);
    }

    Json BreakVec3::AbstractSerialize() {
        return SerializeAllAttributes();
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