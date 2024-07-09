#include "common/common.h"
#include "gpu/gpu.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"

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
};