#include "make_vec2.h"

namespace Raster {

    MakeVec2::MakeVec2() {
        NodeBase::Initialize();

        SetupAttribute("X", 1.0f);
        SetupAttribute("Y", 1.0f);

        AddOutputPin("Output");
    }

    AbstractPinMap MakeVec2::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto xCandidate = GetAttribute<float>("X", t_contextData);
        auto yCandidate = GetAttribute<float>("Y", t_contextData);
        if (xCandidate.has_value() && yCandidate.has_value()) {
            auto& x = xCandidate.value();
            auto& y = yCandidate.value();
            TryAppendAbstractPinMap(result, "Output", glm::vec2(x, y));
        }

        return result;
    }

    void MakeVec2::AbstractRenderProperties() {
        RenderAttributeProperty("X");
        RenderAttributeProperty("Y");
    }

    Json MakeVec2::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void MakeVec2::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    bool MakeVec2::AbstractDetailsAvailable() {
        return false;
    }

    std::string MakeVec2::AbstractHeader() {
        return "Make Vec2";
    }

    std::string MakeVec2::Icon() {
        return ICON_FA_UP_DOWN_LEFT_RIGHT;
    }

    std::optional<std::string> MakeVec2::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MakeVec2>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Make Vec2",
            .packageName = RASTER_PACKAGED "make_vec2",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}