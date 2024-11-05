#include "make_vec3.h"

namespace Raster {

    MakeVec3::MakeVec3() {
        NodeBase::Initialize();

        SetupAttribute("X", 1.0f);
        SetupAttribute("Y", 1.0f);
        SetupAttribute("Z", 1.0f);


        AddOutputPin("Output");
    }

    AbstractPinMap MakeVec3::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto xCandidate = GetAttribute<float>("X", t_contextData);
        auto yCandidate = GetAttribute<float>("Y", t_contextData);
        auto zCandidate = GetAttribute<float>("Z", t_contextData);
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
        return SerializeAllAttributes();
    }

    void MakeVec3::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
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
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MakeVec3>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Make Vec3",
            .packageName = RASTER_PACKAGED "make_vec3",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}