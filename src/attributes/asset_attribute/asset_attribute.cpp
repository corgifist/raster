#include "asset_attribute.h"

namespace Raster {
    AssetAttribute::AssetAttribute() {
        AttributeBase::Initialize();

        keyframes.push_back(
            AttributeKeyframe(
                0,
                (int) 0
            )
        );
    }

    std::any AssetAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        return t_percentage > 0.97f ? std::any_cast<int>(t_endValue) : std::any_cast<int>(t_beginValue);
    }

    void AssetAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }

    Json AssetAttribute::SerializeKeyframeValue(std::any t_value) {
        return std::any_cast<int>(t_value);
    }  

    std::any AssetAttribute::LoadKeyframeValue(Json t_value) {
        return t_value.get<int>();
    }

    std::any AssetAttribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        int assetID = std::any_cast<int>(t_originalValue);
        auto& project = Workspace::GetProject();
        auto assetCandidate = Workspace::GetAssetByAssetID(assetID);
        auto assetDescriptionCandidate = Assets::GetAssetImplementation(assetCandidate.has_value() ? assetCandidate.value()->packageName : "");
        std::string assetChooserButtonText = Localization::GetString("NO_ASSET");
        if (assetCandidate.has_value()) {
            assetChooserButtonText = assetCandidate.value()->name;
        }
        std::string popupID = FormatString("##assetChooserPopup%i", id);
        if (ImGui::Button(FormatString("%s %s", 
                assetDescriptionCandidate.has_value() ? assetDescriptionCandidate.value().description.icon.c_str() 
                : ICON_FA_XMARK, assetChooserButtonText.c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, 0))) {
            ImGui::OpenPopup(popupID.c_str());
        }

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(ASSET_ATTRIBUTE_DRAG_DROP_PAYLOAD, &id, sizeof(id));
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

        if (ImGui::BeginPopup(popupID.c_str())) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_FOLDER_OPEN, Localization::GetString("SELECT_ASSET").c_str()).c_str());
            static std::string s_assetFilter = "";
            ImGui::InputTextWithHint("##assetFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SELECT_ASSET").c_str()).c_str(), &s_assetFilter);
            if (ImGui::BeginChild("##assetCandidates", ImVec2(ImGui::GetContentRegionAvail().x, 300))) {
                for (auto& asset : project.assets) {
                    if (!s_assetFilter.empty() && LowerCase(asset->name).find(LowerCase(s_assetFilter)) == std::string::npos) continue;
                    auto implementationCandidate = Assets::GetAssetImplementation(asset->packageName);
                    if (implementationCandidate.has_value()) {
                        auto& implementation = implementationCandidate.value();
                        ImGui::PushID(asset->id);
                            if (ImGui::Selectable(FormatString("%s %s", implementation.description.icon.c_str(), asset->name.c_str()).c_str())) {
                                assetID = asset->id;
                                isItemEdited = true;
                                ImGui::CloseCurrentPopup();
                            }
                        ImGui::PopID();
                    }
                }
            }
            ImGui::EndChild();
            ImGui::EndPopup();
        }
        return assetID;
    }

    void AssetAttribute::AbstractRenderDetails() {

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