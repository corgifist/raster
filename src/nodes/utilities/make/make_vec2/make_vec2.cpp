#include "make_vec2.h"

namespace Raster {

    MakeVec2::MakeVec2() {
        NodeBase::Initialize();

        SetupAttribute("X", 1.0f);
        SetupAttribute("Y", 1.0f);

        AddOutputPin("Output");
    }

    AbstractPinMap MakeVec2::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        return result;
    }

    void MakeVec2::AbstractRenderProperties() {
        RenderAttributeProperty("X");
        RenderAttributeProperty("Y");
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
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MakeVec2>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Make Vec2",
            .packageName = RASTER_PACKAGED "make_vec2",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}