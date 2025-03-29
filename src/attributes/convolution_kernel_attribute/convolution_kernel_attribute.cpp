#include "common/bezier_curve.h"
#include "common/convolution_kernel.h"
#include "common/localization.h"
#include "common/ui_helpers.h"
#include "font/IconsFontAwesome5.h"
#include "convolution_kernel_attribute.h"
#include "common/dispatchers.h"
#include "raster.h"
#include <glm/gtc/type_ptr.hpp>
#include "../../ImGui/imgui_stdlib.h"

namespace Raster {
    ConvolutionKernelAttribute::ConvolutionKernelAttribute() {
        AttributeBase::Initialize();

        this->interpretAsColor = true;

        keyframes.push_back(
            AttributeKeyframe(
                0,
                ConvolutionKernel()
            )
        );
    }


    std::any ConvolutionKernelAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        return std::any_cast<ConvolutionKernel>(t_beginValue);
    }

    void ConvolutionKernelAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }

    static ImVec2 FitRectInRect(ImVec2 dst, ImVec2 src) {
        float scale = std::min(dst.x / src.x, dst.y / src.y);
        return ImVec2{src.x * scale, src.y * scale};
    }

    std::any ConvolutionKernelAttribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        auto kernel = std::any_cast<ConvolutionKernel>(t_originalValue);

        auto buttonText = FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("EDIT_KERNEL").c_str());
        if (ImGui::Button(buttonText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, 0))) {
            ImGui::OpenPopup("##editKernel");
        }
        if (ImGui::BeginPopup("##editKernel")) {
            ImGui::SeparatorText(FormatString("%s %s: %s", ICON_FA_IMAGE, Localization::GetString("EDIT_VALUE").c_str(), name.c_str()).c_str());
            if (GPU::s_imageConvolutionPreviewTexture.handle && GPU::s_kernelPreviewPipeline.handle) {
                static Framebuffer s_previewFramebuffer;
                auto& previewTexture = GPU::s_imageConvolutionPreviewTexture;
                if (!s_previewFramebuffer.handle) {
                    s_previewFramebuffer = GPU::GenerateFramebuffer(previewTexture.width, previewTexture.height, {GPU::GenerateTexture(previewTexture.width, previewTexture.height, 3)});
                }
                static std::optional<glm::mat3> s_lastRenderedKernel = std::nullopt;
                if (!s_lastRenderedKernel || (s_lastRenderedKernel && *s_lastRenderedKernel != kernel.GetKernel())) {
                    GPU::BindPipeline(GPU::s_kernelPreviewPipeline);
                    GPU::BindFramebuffer(s_previewFramebuffer);
                    GPU::ClearFramebuffer(0, 0, 0, 1);
                    GPU::SetShaderUniform(GPU::s_kernelPreviewPipeline.fragment, "uResolution", glm::vec2(s_previewFramebuffer.width, s_previewFramebuffer.height));
                    GPU::SetShaderUniform(GPU::s_kernelPreviewPipeline.fragment, "uKernel", kernel.GetKernel());
                    GPU::BindTextureToShader(GPU::s_kernelPreviewPipeline.fragment, "uBase", previewTexture, 0);
                    GPU::DrawArrays(3);
                    // RASTER_LOG("rendering kernel preview");
                    s_lastRenderedKernel = kernel.GetKernel();
                }
                auto fitSize = FitRectInRect(ImVec2(200, 200), ImVec2(previewTexture.width, previewTexture.height));
                ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - fitSize.x / 2.0f);
                ImGui::Image((ImTextureID) s_previewFramebuffer.attachments[0].handle, fitSize);
            }
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s %s ", ICON_FA_XMARK, Localization::GetString("MULTIPLIER").c_str());
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat("##multiplierDrag", &kernel.multiplier, 0.01f);
            if (ImGui::IsItemEdited()) isItemEdited = true;
            ImGui::PopItemWidth();
            if (ImGui::IsItemEdited()) isItemEdited = true;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragFloat3("##kernel0", glm::value_ptr(kernel.kernel[0]), 1);
                if (ImGui::IsItemEdited()) isItemEdited = true;
                ImGui::DragFloat3("##kernel1", glm::value_ptr(kernel.kernel[1]), 1);
                if (ImGui::IsItemEdited()) isItemEdited = true;
                ImGui::DragFloat3("##kernel2", glm::value_ptr(kernel.kernel[2]), 1);
                if (ImGui::IsItemEdited()) isItemEdited = true;
            ImGui::PopItemWidth();
            static bool s_searchFocused = false;
            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("KERNEL_PRESETS").c_str()).c_str())) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("KERNEL_PRESETS").c_str()).c_str());
                static std::string s_searchFilter = "";
                if (!s_searchFocused) {
                    ImGui::SetKeyboardFocusHere(0);
                    s_searchFocused = true;
                }
                ImGui::InputTextWithHint("##searchFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_searchFilter);
                if (ImGui::BeginChild("##presets", ImVec2(ImGui::GetContentRegionAvail().x, RASTER_PREFERRED_POPUP_HEIGHT))) {
                    for (auto& preset : ConvolutionKernel::s_presets) {
                        if (!s_searchFilter.empty() && LowerCase(preset.first).find(LowerCase(s_searchFilter)) == std::string::npos) continue;
                        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_IMAGE, preset.first.c_str()).c_str())) {
                            kernel = preset.second;
                            isItemEdited = true;
                        }
                        if (ImGui::BeginItemTooltip()) {
                            std::any dynamicKernel = preset.second;
                            Dispatchers::DispatchString(dynamicKernel);
                            ImGui::EndTooltip();
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::EndMenu();
            } else s_searchFocused = false;
            ImGui::EndPopup();
        }

        return kernel;
    }

    void ConvolutionKernelAttribute::AbstractRenderDetails() {
        auto& project = Workspace::s_project.value();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
        ImGui::PushID(id);
            auto currentValue = Get(project.GetCorrectCurrentTime() - parentComposition->beginFrame, parentComposition);
            Dispatchers::DispatchString(currentValue);
        ImGui::PopID();
    }

    Json ConvolutionKernelAttribute::SerializeKeyframeValue(std::any t_value) {
        return std::any_cast<ConvolutionKernel>(t_value).Serialize();
    }  

    std::any ConvolutionKernelAttribute::LoadKeyframeValue(Json t_value) {
        return ConvolutionKernel(t_value);
    }

}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::ConvolutionKernelAttribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "convolution_kernel_attribute",
            .prettyName = ICON_FA_IMAGE " Convolution Kernel"
        };
    }
}