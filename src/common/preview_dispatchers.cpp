#include "common/common.h"
#include "font/font.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_drag.h"
#include "gpu/gpu.h"

namespace Raster {

    static ImVec2 FitRectInRect(ImVec2 dst, ImVec2 src) {
        float scale = std::min(dst.x / src.x, dst.y / src.y);
        return ImVec2{src.x * scale, src.y * scale};
    }

    void PreviewDispatchers::DispatchStringValue(std::any& t_attribute) {
        static DragStructure textDrag;
        static ImVec2 textOffset;
        static float zoom = 1.0f;

        std::string str = std::any_cast<std::string>(t_attribute);
        if (std::floor(zoom) > 1) {
            ImGui::PushFont(Font::s_denseFont);
        }
        ImGui::SetWindowFontScale(zoom);
            ImVec2 textSize = ImGui::CalcTextSize(str.c_str());
            ImGui::SetCursorPos(ImVec2{
                ImGui::GetContentRegionAvail().x / 2.0f - textSize.x / 2.0f,
                ImGui::GetContentRegionAvail().y / 2.0f - textSize.y / 2.0f
            } + textOffset);
            ImGui::Text(str.c_str());
        ImGui::SetWindowFontScale(1.0f);
        if (std::floor(zoom) > 1) {
            ImGui::PopFont();
        }

        textDrag.Activate();
        float textDragDistance;
        if (textDrag.GetDragDistance(textDragDistance)) {
            textOffset = textOffset + ImGui::GetIO().MouseDelta;
        } else textDrag.Deactivate();

        if (ImGui::GetIO().MouseWheel != 0 && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            zoom += ImGui::GetIO().MouseWheel * 0.1f;
            zoom = std::max(zoom, 0.5f);
        }
    }

    void PreviewDispatchers::DispatchTextureValue(std::any& t_attribute) {
        static DragStructure imageDrag;
        static ImVec2 imageOffset;
        static float zoom = 1.0f;

        Texture texture = std::any_cast<Texture>(t_attribute);
        ImVec2 fitTextureSize = FitRectInRect(ImGui::GetWindowSize(), ImVec2(texture.width, texture.height));

        ImGui::BeginChild("##imageContainer", ImGui::GetContentRegionAvail(), 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::SetCursorPos(ImVec2{
                ImGui::GetWindowSize().x / 2.0f - fitTextureSize.x * zoom / 2,
                ImGui::GetWindowSize().y / 2.0f - fitTextureSize.y * zoom / 2
            } + imageOffset);
            ImGui::Image(texture.handle, fitTextureSize * zoom);

            auto footerText = FormatString("%ix%i; %s", (int) texture.width, (int) texture.height, texture.PrecisionToString().c_str());
            ImVec2 footerSize = ImGui::CalcTextSize(footerText.c_str());
            ImGui::SetCursorPos({
                ImGui::GetWindowSize().x / 2.0f - footerSize.x / 2.0f,
                ImGui::GetWindowSize().y - footerSize.y
            });
            ImGui::Text(footerText.c_str());

            imageDrag.Activate();
            float imageDragDistance;
            if (imageDrag.GetDragDistance(imageDragDistance)) {
                imageOffset = imageOffset + ImGui::GetIO().MouseDelta;
            } else imageDrag.Deactivate();
        ImGui::EndChild();

        if (ImGui::GetIO().MouseWheel != 0 && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            zoom += ImGui::GetIO().MouseWheel * 0.1f;
            zoom = std::max(zoom, 0.5f);
        }
    }
};
