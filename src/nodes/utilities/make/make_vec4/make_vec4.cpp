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

    AbstractPinMap MakeVec4::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto xCandidate = GetAttribute<float>("X", t_contextData);
        auto yCandidate = GetAttribute<float>("Y", t_contextData);
        auto zCandidate = GetAttribute<float>("Z", t_contextData);
        auto wCandidate = GetAttribute<float>("W", t_contextData);
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
        return SerializeAllAttributes();
    }

    void MakeVec4::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
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
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MakeVec4>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Make Vec4",
            .packageName = RASTER_PACKAGED "make_vec4",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}