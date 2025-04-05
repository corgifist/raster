#include "colorspace_attribute.h"
#include "common/ui_helpers.h"
#include "common/asset_id.h"
#include "common/color_management.h"
#include "common/colorspace.h"
#include "common/dispatchers.h"

namespace Raster {
    ColorspaceAttribute::ColorspaceAttribute() {
        AttributeBase::Initialize();

        keyframes.push_back(
            AttributeKeyframe(
                0,
                Colorspace()
            )
        );
    }

    std::any ColorspaceAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        return t_beginValue;
    }

    void ColorspaceAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }

    Json ColorspaceAttribute::SerializeKeyframeValue(std::any t_value) {
        return std::any_cast<Colorspace>(t_value).name;
    }  

    std::any ColorspaceAttribute::LoadKeyframeValue(Json t_value) {
        return Colorspace(t_value.get<std::string>());
    }

    std::any ColorspaceAttribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        auto& project = Workspace::GetProject();
        auto colorspace = std::any_cast<Colorspace>(t_originalValue);
        if (ImGui::Button(FormatString("%s %s", ICON_FA_DROPLET, colorspace.name.c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, 0))) {
            ImGui::OpenPopup("##colorspacePopup");
        }
        if (ImGui::BeginPopup("##colorspacePopup")) {
            ImGui::SeparatorText(FormatString("%s %s: %s", ICON_FA_DROPLET, Localization::GetString("EDIT_VALUE").c_str(), name.c_str()).c_str());
            static std::string s_searchFilter = "";
            ImGui::InputTextWithHint("##searchFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_searchFilter);
            if (ImGui::BeginChild("##colorspaceCandidates", ImVec2(ImGui::GetContentRegionAvail().x, RASTER_PREFERRED_POPUP_HEIGHT))) {
                for (auto& cs : ColorManagement::s_colorspaces) {
                    if (!s_searchFilter.empty() && LowerCase(cs).find(LowerCase(s_searchFilter)) == std::string::npos) continue;
                    if (ImGui::MenuItem(FormatString("%s%s %s", ICON_FA_DISPLAY, cs == colorspace.name ? " " ICON_FA_CHECK : "", cs.c_str()).c_str())) {
                        colorspace = Colorspace(cs);
                        isItemEdited = true;
                        ImGui::CloseCurrentPopup();
                    }
                }   
            }
            ImGui::EndChild();
            ImGui::EndPopup();
        }

        return colorspace;
    }

    void ColorspaceAttribute::AbstractRenderDetails() {
        auto& project = Workspace::s_project.value();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
        ImGui::PushID(id);
            auto currentValue = Get(project.GetCorrectCurrentTime() - parentComposition->GetBeginFrame(), parentComposition);
            Dispatchers::DispatchString(currentValue);
        ImGui::PopID();
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::ColorspaceAttribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "colorspace_attribute",
            .prettyName = ICON_FA_DISPLAY " Colorspace"
        };
    }
}