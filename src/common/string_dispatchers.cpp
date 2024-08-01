#include "common/common.h"
#include "gpu/gpu.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"
#include "../ImGui/imgui_drag.h"
#include "common/overlay_dispatchers.h"
#include "common/transform2d.h"

namespace Raster {

    static ImVec2 FitRectInRect(ImVec2 dst, ImVec2 src) {
        float scale = std::min(dst.x / src.x, dst.y / src.y);
        return ImVec2{src.x * scale, src.y * scale};
    }

    void StringDispatchers::DispatchStringValue(std::any& t_attribute) {
        ImGui::Text("%s %s: '%s'", ICON_FA_QUOTE_LEFT, Localization::GetString("VALUE").c_str(), std::any_cast<std::string>(t_attribute).c_str());
    }

    void StringDispatchers::DispatchTextureValue(std::any& t_attribute) {
        auto texture = std::any_cast<Texture>(t_attribute);
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - 64);
        ImGui::Image(texture.handle, FitRectInRect(ImVec2(128, 128), ImVec2(texture.width, texture.height)));
        
        auto footerText = FormatString("%ix%i; %s", (int) texture.width, (int) texture.height, texture.PrecisionToString().c_str());
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(footerText.c_str()).x / 2.0f);
        ImGui::Text(footerText.c_str());
    }

    void StringDispatchers::DispatchFloatValue(std::any& t_attribute) {
        ImGui::Text("%s %s: %0.2f", ICON_FA_CIRCLE_INFO, Localization::GetString("VALUE").c_str(), std::any_cast<float>(t_attribute));
    }

    void StringDispatchers::DispatchIntValue(std::any& t_attribute) {
        ImGui::Text("%s %s: %i", ICON_FA_CIRCLE_INFO, Localization::GetString("VALUE").c_str(), std::any_cast<int>(t_attribute));
    }

    void StringDispatchers::DispatchVector4Value(std::any& t_attribute) {
        auto vector = std::any_cast<glm::vec4>(t_attribute);
        ImGui::Text("%s %s: (%0.2f; %0.2f; %0.2f; %0.2f)", ICON_FA_CIRCLE_INFO, Localization::GetString("VALUE").c_str(), vector.x, vector.y, vector.z, vector.w);
        ImGui::PushItemWidth(200);
            ImGui::ColorPicker4("##colorPreview", glm::value_ptr(vector), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
        ImGui::PopItemWidth();
    }

    void StringDispatchers::DispatchFramebufferValue(std::any& t_attribute) {
        auto framebuffer = std::any_cast<Framebuffer>(t_attribute);
        ImGui::Text("%s %s: %i", ICON_FA_IMAGE, Localization::GetString("ATTACHMENTS_COUNT").c_str(), (int) framebuffer.attachments.size());
        ImGui::Separator();
        ImGui::Spacing();
        for (auto& attachment : framebuffer.attachments) {
            std::any dynamicAttachment = attachment;
            ImGui::BeginChild(FormatString("##%i", (int) (uint64_t) attachment.handle).c_str(), ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
                DispatchTextureValue(dynamicAttachment);
            ImGui::EndChild();
            ImGui::Separator();
            ImGui::Spacing();
        }
    }

    void StringDispatchers::DispatchSamplerSettingsValue(std::any& t_attribute) {
        auto samplerSettings = std::any_cast<SamplerSettings>(t_attribute);
        ImGui::Text("%s %s: %s", ICON_FA_IMAGE, Localization::GetString("FILTERING_MODE").c_str(), GPU::TextureFilteringModeToString(samplerSettings.filteringMode).c_str());
        ImGui::Text("%s %s: %s", ICON_FA_IMAGE, Localization::GetString("WRAPPING_MODE").c_str(), GPU::TextureWrappingModeToString(samplerSettings.wrappingMode).c_str());
    }

    void StringDispatchers::DispatchTransform2DValue(std::any& t_attribute) {
        auto transform = std::any_cast<Transform2D>(t_attribute);
        std::any transformCopy = transform;
        auto preferredResolution = Workspace::GetProject().preferredResolution;
        ImVec2 fitSize = FitRectInRect(ImVec2{128, ImGui::GetWindowSize().y}, ImVec2{preferredResolution.x, preferredResolution.y});
        ImGui::BeginChild("##transformInfoContainer", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
            ImGui::Text("%s Position: %0.2f; %0.2f", ICON_FA_UP_DOWN_LEFT_RIGHT, transform.position.x, transform.position.y);
            ImGui::Text("%s Size: %0.2f; %0.2f", ICON_FA_SCALE_BALANCED, transform.size.x, transform.size.y);
            ImGui::Text("%s Anchor: %0.2f; %0.2f", ICON_FA_ANCHOR, transform.anchor.x, transform.anchor.y);
            ImGui::Text("%s Angle: %0.2f", ICON_FA_ROTATE, transform.angle);
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("##transformPreviewContainer", fitSize);
            RectBounds backgroundBounds(
                ImVec2(0, 0), 
                fitSize
            );
            ImGui::GetWindowDrawList()->AddRectFilled(backgroundBounds.UL, backgroundBounds.BR, ImGui::GetColorU32(ImVec4(0, 0, 0, 1)));
            OverlayDispatchers::DispatchTransform2DValue(transformCopy, nullptr, -1, 1.0f, {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y});
        ImGui::EndChild();
    }

    void StringDispatchers::DispatchBoolValue(std::any& t_attribute) {
        bool value = std::any_cast<bool>(t_attribute);
        ImGui::Text("%s %s", ICON_FA_CIRCLE_INFO, value ? "true" : "false");
    }
};