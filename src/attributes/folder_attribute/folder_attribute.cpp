#include "folder_attribute.h"
#include "common/attribute.h"
#include "font/IconsFontAwesome5.h"

namespace Raster {
    FolderAttribute::FolderAttribute() {
        AttributeBase::Initialize();

        keyframes.push_back(
            AttributeKeyframe(
                0,
                std::nullopt
            )
        );
    }

    Json FolderAttribute::AbstractSerialize() {
        Json result = Json::array();
        for (auto& attribute : attributes) {
            result.push_back(attribute->Serialize());
        }
        return result;
    }

    void FolderAttribute::Load(Json t_data) {
        for (auto& attribute : t_data) {
            auto attributeCandidate = Attributes::InstantiateSerializedAttribute(attribute);
            if (attributeCandidate) {
                attributes.push_back(*attributeCandidate);
            }
        }
    }

    std::optional<std::vector<AbstractAttribute>*> FolderAttribute::AbstractGetChildAttributes() {
        return &attributes;
    }

    std::any FolderAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        return std::nullopt;
    }

    void FolderAttribute::RenderKeyframes() {
    }


    std::any FolderAttribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) { 
        return std::nullopt;
    }

    void FolderAttribute::AbstractRenderDetails() {
        if (!attributes.empty()) {
            attributes[0]->AbstractRenderDetails();
        }
    }


    Json FolderAttribute::SerializeKeyframeValue(std::any t_value) {
        return {};
    }  

    std::any FolderAttribute::LoadKeyframeValue(Json t_value) {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::FolderAttribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "folder_attribute",
            .prettyName = ICON_FA_FOLDER_OPEN " Folder"
        };
    }
}