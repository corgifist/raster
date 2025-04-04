#include "asset_attribute.h"
#include "common/ui_helpers.h"
#include "common/asset_id.h"

namespace Raster {
    AssetAttribute::AssetAttribute() {
        AttributeBase::Initialize();

        keyframes.push_back(
            AttributeKeyframe(
                0,
                AssetID(0)
            )
        );
    }

    std::any AssetAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        return t_percentage > 0.97f ? t_endValue : t_beginValue;
    }

    void AssetAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }

    Json AssetAttribute::SerializeKeyframeValue(std::any t_value) {
        return std::any_cast<AssetID>(t_value).id;
    }  

    std::any AssetAttribute::LoadKeyframeValue(Json t_value) {
        return AssetID(t_value.get<int>());
    }

    std::any AssetAttribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        int assetID = std::any_cast<AssetID>(t_originalValue).id;
        auto& project = Workspace::GetProject();
        auto assetCandidate = Workspace::GetAssetByAssetID(assetID);
        auto assetDescriptionCandidate = Assets::GetAssetImplementationByPackageName(assetCandidate.has_value() ? assetCandidate.value()->packageName : "");
        std::string assetChooserButtonText = Localization::GetString("NO_ASSET");
        if (assetCandidate.has_value()) {
            assetChooserButtonText = assetCandidate.value()->name;
        }
        std::string popupID = FormatString("##assetChooserPopup%i", id);
        bool openAssetPopup = false;
        ImVec4 colorMarkNormal = assetCandidate ? ImGui::ColorConvertU32ToFloat4((*assetCandidate)->colorMark) : ImGui::GetStyleColorVec4(ImGuiCol_Button);
        ImVec4 colorMarkHovered = assetCandidate ? ImGui::ColorConvertU32ToFloat4((*assetCandidate)->colorMark) * 1.1f : ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
        ImVec4 colorMarkPressed = assetCandidate ? ImGui::ColorConvertU32ToFloat4((*assetCandidate)->colorMark) * 1.3f : ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
        ImGui::PushStyleColor(ImGuiCol_Button, colorMarkNormal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorMarkHovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorMarkPressed);
        if (ImGui::Button(FormatString("%s %s", 
                assetDescriptionCandidate.has_value() ? assetDescriptionCandidate.value().description.icon.c_str() 
                : ICON_FA_XMARK, assetChooserButtonText.c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, 0))) {
            openAssetPopup = true;
        }
        ImGui::PopStyleColor(3);

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(ASSET_ATTRIBUTE_DRAG_DROP_PAYLOAD, &id, sizeof(id));
            if (assetCandidate) {
                (*assetCandidate)->RenderDetails();
            }
            ImGui::EndDragDropSource();
        }
        
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD)) {
                int payloadAssetID = *((int*) payload->Data);
                assetID = payloadAssetID;
                isItemEdited = true;
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PushID(id);
            static std::string assetSearchFilter = "";
            if (openAssetPopup) {
                UIHelpers::OpenSelectAssetPopup();
            }
            int reservedAssetID = assetID;
            UIHelpers::SelectAsset(assetID, assetCandidate.has_value() ? assetCandidate.value()->name : "", &assetSearchFilter);
            if (reservedAssetID != assetID) {
                isItemEdited = true;
            }
        ImGui::PopID();
        return AssetID(assetID);
    }

    void AssetAttribute::AbstractRenderDetails() {
        if (!Workspace::IsProjectLoaded()) return;
        auto& project = Workspace::GetProject();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
        auto assetID = std::any_cast<AssetID>(Get(project.GetCorrectCurrentTime() - parentComposition->GetBeginFrame(), parentComposition));
        auto assetCandidate = Workspace::GetAssetByAssetID(assetID.id);
        if (assetCandidate.has_value()) {
            assetCandidate.value()->RenderDetails();
        }
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::AssetAttribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "asset_attribute",
            .prettyName = ICON_FA_FOLDER_OPEN " Asset"
        };
    }
}