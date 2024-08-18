#include "make_vec4.h"

namespace Raster {

    MakeVec4::MakeVec4() {
        NodeBase::Initialize();

        SetupAttribute("X", 1.0f);
        SetupAttribute("Y", 1.0f);
        SetupAttribute("Z", 1.0f);
        SetupAttribute("W", 1.0f);

        AddOutputPin("Output");
    }

    AbstractPinMap MakeVec4::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto xCandidate = GetAttribute<float>("X");
        auto yCandidate = GetAttribute<float>("Y");
        auto zCandidate = GetAttribute<float>("Z");
        auto wCandidate = GetAttribute<float>("W");
        if (xCandidate.has_value() && yCandidate.has_value() && zCandidate.has_value() && wCandidate.has_value()) {
            auto& x = xCandidate.value();
            auto& y = yCandidate.value();
            auto& z = zCandidate.value();
            auto& w = wCandidate.value();
            TryAppendAbstractPinMap(result, "Output", glm::vec4(x, y, z, w));
        }

        return result;
    }

    void MakeVec4::AbstractRenderProperties() {
        RenderAttributeProperty("X");
        RenderAttributeProperty("Y");
        RenderAttributeProperty("Z");
        RenderAttributeProperty("W");
    }

    Json MakeVec4::AbstractSerialize() {
        return {
            {"X", RASTER_ATTRIBUTE_CAST(float, "X")},
            {"Y", RASTER_ATTRIBUTE_CAST(float, "Y")},
            {"Z", RASTER_ATTRIBUTE_CAST(float, "Z")},
            {"W", RASTER_ATTRIBUTE_CAST(float, "W")}
        };
    }

    void MakeVec4::AbstractLoadSerialized(Json t_data) {
        SetAttributeValue("X", t_data["X"].get<float>());
        SetAttributeValue("Y", t_data["Y"].get<float>());  
        SetAttributeValue("Z", t_data["Z"].get<float>());
        SetAttributeValue("W", t_data["W"].get<float>());
    }


    bool MakeVec4::AbstractDetailsAvailable() {
        return false;
    }

    std::string MakeVec4::AbstractHeader() {
        return "Make Vec4";
    }

    std::string MakeVec4::Icon() {
        return ICON_FA_UP_DOWN_LEFT_RIGHT;
    }

    std::optional<std::string> MakeVec4::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MakeVec4>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Make Vec4",
            .packageName = RASTER_PACKAGED "make_vec4",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}