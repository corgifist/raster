#include "break_transform2d.h"

namespace Raster {

    BreakTransform2D::BreakTransform2D() {
        NodeBase::Initialize();

        SetupAttribute("Transform", Transform2D());

        AddOutputPin("Position");
        AddOutputPin("Size");
        AddOutputPin("Anchor");
        AddOutputPin("Angle");
        AddOutputPin("ParentTransform");
    }

    AbstractPinMap BreakTransform2D::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto transformCandidate = GetAttribute<Transform2D>("Transform");
        if (transformCandidate.has_value()) {
            auto& transform = transformCandidate.value();
            TryAppendAbstractPinMap(result, "Position", transform.position);
            TryAppendAbstractPinMap(result, "Size", transform.size);
            TryAppendAbstractPinMap(result, "Angle", transform.angle);
            TryAppendAbstractPinMap(result, "Anchor", transform.anchor);
            if (transform.parentTransform) {
                TryAppendAbstractPinMap(result, "ParentTransform", *transform.parentTransform);
            }
        }
        return result;
    }

    void BreakTransform2D::AbstractRenderProperties() {
        RenderAttributeProperty("Transform");
    }

    bool BreakTransform2D::AbstractDetailsAvailable() {
        return false;
    }

    std::string BreakTransform2D::AbstractHeader() {
        return "Break Transform2D";
    }

    std::string BreakTransform2D::Icon() {
        return ICON_FA_UP_DOWN_LEFT_RIGHT;
    }

    std::optional<std::string> BreakTransform2D::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::BreakTransform2D>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Break Transform2D",
            .packageName = RASTER_PACKAGED "break_transform2d",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}