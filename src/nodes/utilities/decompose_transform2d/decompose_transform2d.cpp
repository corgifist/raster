#include "decompose_transform2d.h"
#include "../../../ImGui/imgui.h"


namespace Raster {

    DecomposeTransform2D::DecomposeTransform2D() {
        NodeBase::Initialize();

        SetupAttribute("Transform", Transform2D());

        AddOutputPin("Position");
        AddOutputPin("Size");
        AddOutputPin("Angle");
    }

    AbstractPinMap DecomposeTransform2D::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto transformCandidate = GetAttribute<Transform2D>("Transform", t_contextData);
        if (transformCandidate.has_value()) {
            auto& transform = transformCandidate.value();
            TryAppendAbstractPinMap(result, "Position", transform.DecomposePosition());
            TryAppendAbstractPinMap(result, "Size", transform.DecomposeSize());
            TryAppendAbstractPinMap(result, "Angle", transform.DecomposeRotation());
        }
        return result;
    }

    void DecomposeTransform2D::AbstractRenderProperties() {
        ImGui::Text("%s Not intended to be an equivalent to 'Break Transform2D' node!", ICON_FA_TRIANGLE_EXCLAMATION);
        ImGui::Text("%s This node decomposes 4x4 transformation matrix produced by Transform2D object", ICON_FA_CIRCLE_QUESTION);
        ImGui::Text("Meanwhile 'Break Transform2D' node returns fields of Transform2D object");
        RenderAttributeProperty("Transform");
    }

    Json DecomposeTransform2D::AbstractSerialize() {
        return SerializeAllAttributes();
    }
    
    void DecomposeTransform2D::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data); 
    }

    bool DecomposeTransform2D::AbstractDetailsAvailable() {
        return false;
    }

    std::string DecomposeTransform2D::AbstractHeader() {
        return "Decompose Transform2D";
    }

    std::string DecomposeTransform2D::Icon() {
        return ICON_FA_UP_DOWN_LEFT_RIGHT;
    }

    std::optional<std::string> DecomposeTransform2D::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::DecomposeTransform2D>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Decompose Transform2D",
            .packageName = RASTER_PACKAGED "decompose_transform2d",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}