#include "make_vec3.h"

namespace Raster {

    MakeVec3::MakeVec3() {
        NodeBase::Initialize();

        SetupAttribute("X", 1.0f);
        SetupAttribute("Y", 1.0f);
        SetupAttribute("Z", 1.0f);


        AddOutputPin("Output");
    }

    AbstractPinMap MakeVec3::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto xCandidate = GetAttribute<float>("X");
        auto yCandidate = GetAttribute<float>("Y");
        auto zCandidate = GetAttribute<float>("Z");
        if (xCandidate.has_value() && yCandidate.has_value() && zCandidate.has_value()) {
            auto& x = xCandidate.value();
            auto& y = yCandidate.value();
            auto& z = zCandidate.value();
            TryAppendAbstractPinMap(result, "Output", glm::vec3(x, y, z));
        }

        return result;
    }

    void MakeVec3::AbstractRenderProperties() {
        RenderAttributeProperty("X");
        RenderAttributeProperty("Y");
        RenderAttributeProperty("Z");
    }

    Json MakeVec3::AbstractSerialize() {
        return {
            {"X", RASTER_ATTRIBUTE_CAST(float, "X")},
            {"Y", RASTER_ATTRIBUTE_CAST(float, "Y")},
            {"Z", RASTER_ATTRIBUTE_CAST(float, "Z")}
        };
    }

    void MakeVec3::AbstractLoadSerialized(Json t_data) {
        SetAttributeValue("X", t_data["X"].get<float>());
        SetAttributeValue("Y", t_data["Y"].get<float>());  
        SetAttributeValue("Z", t_data["Z"].get<float>());
    }

    bool MakeVec3::AbstractDetailsAvailable() {
        return false;
    }

    std::string MakeVec3::AbstractHeader() {
        return "Make Vec4";
    }

    std::string MakeVec3::Icon() {
        return ICON_FA_UP_DOWN_LEFT_RIGHT;
    }

    std::optional<std::string> MakeVec3::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MakeVec3>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Make Vec3",
            .packageName = RASTER_PACKAGED "make_vec3",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}