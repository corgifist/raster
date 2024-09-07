#include "break_vec4.h"

namespace Raster {

    BreakVec4::BreakVec4() {
        NodeBase::Initialize();

        SetupAttribute("Vector", glm::vec4());

        AddOutputPin("X");
        AddOutputPin("Y");
        AddOutputPin("Z");
        AddOutputPin("W");
    }

    AbstractPinMap BreakVec4::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto vectorCandidate = GetAttribute<glm::vec4>("Vector");
        if (vectorCandidate.has_value()) {
            auto& vector = vectorCandidate.value();
            TryAppendAbstractPinMap(result, "X", vector.x);
            TryAppendAbstractPinMap(result, "Y", vector.y);
            TryAppendAbstractPinMap(result, "Z", vector.z);
            TryAppendAbstractPinMap(result, "W", vector.w);
        }
        return result;
    }

    void BreakVec4::AbstractRenderProperties() {
        RenderAttributeProperty("Vector");
    }
    
    void BreakVec4::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);    
    }

    Json BreakVec4::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool BreakVec4::AbstractDetailsAvailable() {
        return false;
    }

    std::string BreakVec4::AbstractHeader() {
        return "Break Vec4";
    }

    std::string BreakVec4::Icon() {
        return ICON_FA_EXPAND;
    }

    std::optional<std::string> BreakVec4::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::BreakVec4>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Break Vec4",
            .packageName = RASTER_PACKAGED "break_vec4",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}