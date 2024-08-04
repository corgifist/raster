#include "common/common.h"
#include "font/font.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_drag.h"
#include "gpu/gpu.h"
#include "overlay_dispatchers.h"
#include "preview_dispatchers.h"
#include "common/transform2d.h"
#include "common/dispatchers.h"

namespace Raster {

    static ImVec2 FitRectInRect(ImVec2 dst, ImVec2 src) {
        float scale = std::min(dst.x / src.x, dst.y / src.y);
        return ImVec2{src.x * scale, src.y * scale};
    }

    static std::vector<std::string> SplitString(std::string s, std::string delimiter) {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
            token = s.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back (token);
        }

        res.push_back (s.substr (pos_start));
        return res;
    }

    void PreviewDispatchers::DispatchStringValue(std::any& t_attribute) {
        static DragStructure textDrag;
        static ImVec2 textOffset;
        static float zoom = 1.0f;

        std::string str = std::any_cast<std::string>(t_attribute);
        if (zoom > 1) {
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
        if (zoom > 1) {
            ImGui::PopFont();
        }

        int lines = SplitString(str, "\n").size();
        std::string footerText = FormatString("%i %s; %i %s; UTF-8", (int) str.size(), Localization::GetString("CHARS").c_str(), lines, Localization::GetString("LINES").c_str());
        ImVec2 footerSize = ImGui::CalcTextSize(footerText.c_str());
        ImGui::SetCursorPos({
            ImGui::GetWindowSize().x / 2.0f - footerSize.x / 2.0f,
            ImGui::GetWindowSize().y - footerSize.y - ImGui::GetStyle().WindowPadding.x
        });
        ImGui::Text(footerText.c_str());

        textDrag.Activate();
        float textDragDistance;
        if (textDrag.GetDragDistance(textDragDistance)) {
            textOffset = textOffset + ImGui::GetIO().MouseDelta;
        } else textDrag.Deactivate();

        if (ImGui::GetIO().MouseWheel != 0 && ImGui::IsWindowFocused()) {
            zoom += ImGui::GetIO().MouseWheel * 0.1f;
            zoom = std::max(zoom, 0.5f);
        }

        if (ImGui::IsWindowFocused() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("##stringPreviewPopup");
        } 
        if (ImGui::BeginPopup("##stringPreviewPopup")) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_FONT, Localization::GetString("ATTRIBUTE").c_str()).c_str());
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_FONT, Localization::GetString("REVERT_VIEW").c_str()).c_str())) {
                textOffset = {0, 0};
                zoom = 1.0f;
            }
            ImGui::EndPopup();
        }
    }

    void PreviewDispatchers::DispatchTextureValue(std::any& t_attribute) {
        static DragStructure imageDrag;
        static ImVec2 imageOffset;
        static float zoom = 1.0f;
        bool imageDragAllowed = true;

        Texture texture = std::any_cast<Texture>(t_attribute);
        ImVec2 fitTextureSize = FitRectInRect(ImGui::GetWindowSize(), ImVec2(texture.width, texture.height));

        ImGui::BeginChild("##imageContainer", ImGui::GetContentRegionAvail(), 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            static bool maskR = true;
            static bool maskG = true;
            static bool maskB = true;
            static bool maskA = true;

            ImGui::SetCursorPos(ImVec2{
                ImGui::GetWindowSize().x / 2.0f - fitTextureSize.x * zoom / 2,
                ImGui::GetWindowSize().y / 2.0f - fitTextureSize.y * zoom / 2
            } + imageOffset);
            ImGui::Image(texture.handle, fitTextureSize * zoom, ImVec2(0, 0), ImVec2(1, 1), ImVec4((int) maskR, (int) maskG, (int) maskB, (int) maskA));

            auto& project = Workspace::GetProject();
            if (!project.selectedAttributes.empty()) {
                for (auto& attributeID : project.selectedAttributes) {
                    auto attributeCandidate = Workspace::GetAttributeByAttributeID(attributeID);
                    auto compositionCandidate = Workspace::GetCompositionByAttributeID(attributeID);
                    if (attributeCandidate.has_value() && compositionCandidate.has_value()) {
                        auto& attribute = attributeCandidate.value();
                        auto& composition = compositionCandidate.value();
                        auto& project = Workspace::s_project.value();
                        auto value = attribute->Get(project.currentFrame - composition->beginFrame, composition);
                        ImVec2 zoomedSize = fitTextureSize * zoom;
                        ImGui::SetCursorPos(ImVec2{
                            ImGui::GetWindowSize().x / 2.0f - fitTextureSize.x * zoom / 2,
                            ImGui::GetWindowSize().y / 2.0f - fitTextureSize.y * zoom / 2
                        } + imageOffset);
                        if (!Dispatchers::DispatchOverlay(value, composition, attribute->id, zoom, {zoomedSize.x, zoomedSize.y})) {
                            imageDragAllowed = false;
                        }
                    }
                }
            }

            auto footerText = FormatString("%ix%i; %s (%s); %s %0.3f", (int) texture.width, (int) texture.height, texture.PrecisionToString().c_str(), texture.GetShortPrecisionInfo().c_str(), ICON_FA_IMAGE, (float) texture.width / (float) texture.height);
            ImVec2 footerSize = ImGui::CalcTextSize(footerText.c_str());
            ImGui::SetCursorPos({0, 0});
            float shadowHeight = 30;
            int shadowAlpha = 128;
            RectBounds shadowBounds(
                ImVec2(0, ImGui::GetWindowSize().y - shadowHeight),
                ImVec2(ImGui::GetWindowSize().x, shadowHeight)
            );
            ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
                shadowBounds.UL, shadowBounds.BR,
                IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), 
                IM_COL32(0, 0, 0, shadowAlpha), IM_COL32(0, 0, 0, shadowAlpha)
            );
            ImGui::SetCursorPos({
                ImGui::GetWindowSize().x / 2.0f - footerSize.x / 2.0f,
                ImGui::GetWindowSize().y - footerSize.y
            });
            ImGui::Text(footerText.c_str());

            ImGui::SetCursorPos({0, 0});
            static bool colorMaskExpanded = false;
            if (ImGui::Button(FormatString("%s %s", ICON_FA_DROPLET, colorMaskExpanded ? ICON_FA_ANGLE_DOWN : ICON_FA_ANGLE_RIGHT).c_str())) {
                colorMaskExpanded = !colorMaskExpanded;
            }
            if (colorMaskExpanded) {
                ImGui::SameLine();
                ImVec4 reservedButtonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
                ImGui::PushStyleColor(ImGuiCol_Button, maskR ? ImVec4(1, 0, 0, 1) : reservedButtonColor * 0.7f);
                if (ImGui::Button("R")) maskR = !maskR;
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button, maskG ? ImVec4(0, 1, 0, 1) : reservedButtonColor * 0.7f);
                if (ImGui::Button("G")) maskG = !maskG;
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button, maskB ? ImVec4(0, 0, 1, 1) : reservedButtonColor * 0.7f);
                if (ImGui::Button("B")) maskB = !maskB;
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button, maskA ? reservedButtonColor : reservedButtonColor * 0.5f);
                if (ImGui::Button("A")) maskA = !maskA;
                ImGui::SameLine();

                ImGui::PopStyleColor(4);
            }

            imageDrag.Activate();
            float imageDragDistance;
            if (imageDrag.GetDragDistance(imageDragDistance) && imageDragAllowed) {
                imageOffset = imageOffset + ImGui::GetIO().MouseDelta;
            } else imageDrag.Deactivate();
            if (ImGui::GetIO().MouseWheel != 0 && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
                zoom += ImGui::GetIO().MouseWheel * 0.1f;
                zoom = std::max(zoom, 0.5f);
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
                ImGui::OpenPopup("##texturePopup");
            }
            if (ImGui::BeginPopup("##texturePopup")) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("ATTRIBUTE").c_str()).c_str());
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("REVERT_VIEW").c_str()).c_str())) {
                    zoom = 1.0f;
                    imageOffset = ImVec2(0, 0);
                }
                ImGui::EndPopup();
            }
        ImGui::EndChild();
    }

    void PreviewDispatchers::DispatchFloatValue(std::any& t_attribute) {
        static ImVec2 plotSize = ImVec2(200, 40);

        ImGui::SetCursorPos({
            ImGui::GetWindowSize().x / 2.0f - plotSize.x / 2.0f,
            ImGui::GetWindowSize().y / 2.0f - plotSize.y / 2.0f
        });
        float value = std::any_cast<float>(t_attribute);
        ImGui::PlotVar("", value);
        ImGui::SetItemTooltip("%s %s", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("INACCURATE_RESULTS_WITH_LARGE_NUMBERS").c_str());
    }

    void PreviewDispatchers::DispatchIntValue(std::any& t_attribute) {
        std::any placeholder = (float) std::any_cast<int>(t_attribute);
        DispatchFloatValue(placeholder);
    }

    void PreviewDispatchers::DispatchVector4Value(std::any& t_attribute) {
        auto vector = std::any_cast<glm::vec4>(t_attribute);
        static bool interpretAsColor = false;
        static float interpreterModeSizeX = 0;

        ImVec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        ImVec4 reservedButtonColor = buttonColor;

        if (!interpretAsColor) buttonColor = buttonColor * 1.2f;
        buttonColor.w = 1.0f;

        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - interpreterModeSizeX / 2.0f);

        float beginCursorX = ImGui::GetCursorPosX();
        ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
        if (ImGui::Button(FormatString("%s %s", interpretAsColor ? ICON_FA_EXPAND : ICON_FA_CHECK, Localization::GetString("VECTOR").c_str()).c_str())) {
            interpretAsColor = false;
        }
        buttonColor = reservedButtonColor;

        ImGui::SameLine();

        if (interpretAsColor) buttonColor = buttonColor * 1.2f;
        buttonColor.w = 1.0f;
        ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
        if (ImGui::Button(FormatString("%s %s", interpretAsColor ? ICON_FA_CHECK : ICON_FA_DROPLET, Localization::GetString("COLOR").c_str()).c_str())) {
            interpretAsColor = true;
        }
        buttonColor = reservedButtonColor;

        ImGui::PopStyleColor(2);
        ImGui::SameLine();


        interpreterModeSizeX = ImGui::GetCursorPosX() - beginCursorX;

        ImGui::NewLine();

        static ImVec2 childSize = ImVec2(100, 100);
        ImGui::SetCursorPos(ImVec2{
            ImGui::GetWindowSize().x / 2.0f - childSize.x / 2.0f,
            ImGui::GetWindowSize().y / 2.0f - childSize.y / 2.0f
        });

        ImGui::BeginChild("##vector4Container", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
        if (!interpretAsColor) {
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1, 0, 0, 1));
            ImGui::PlotVar(FormatString("%s %s", ICON_FA_STOPWATCH, "(x)").c_str(), vector.x);

            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0, 1, 0, 1));
            ImGui::PlotVar(FormatString("%s %s", ICON_FA_STOPWATCH, "(y)").c_str(), vector.y);

            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0, 0, 1, 1));
            ImGui::PlotVar(FormatString("%s %s", ICON_FA_STOPWATCH, "(z)").c_str(), vector.z);

            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1, 1, 0, 1));
            ImGui::PlotVar(FormatString("%s %s", ICON_FA_STOPWATCH, "(w)").c_str(), vector.w);
            ImGui::PopStyleColor(4);
        } else {
            float vectorPtr[4] = {
                vector.x,
                vector.y,
                vector.z,
                vector.w
            };
            ImGui::PushItemWidth(200);
                ImGui::ColorPicker4("##colorPreview", vectorPtr, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
            ImGui::PopItemWidth();
        }
        childSize = ImGui::GetWindowSize();
        ImGui::EndChild();
    }

    void PreviewDispatchers::DispatchVector3Value(std::any& t_attribute) {
        auto vector = std::any_cast<glm::vec3>(t_attribute);
        static bool interpretAsColor = false;
        static float interpreterModeSizeX = 0;

        ImVec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        ImVec4 reservedButtonColor = buttonColor;

        if (!interpretAsColor) buttonColor = buttonColor * 1.2f;
        buttonColor.w = 1.0f;

        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - interpreterModeSizeX / 2.0f);

        float beginCursorX = ImGui::GetCursorPosX();
        ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
        if (ImGui::Button(FormatString("%s %s", interpretAsColor ? ICON_FA_EXPAND : ICON_FA_CHECK, Localization::GetString("VECTOR").c_str()).c_str())) {
            interpretAsColor = false;
        }
        buttonColor = reservedButtonColor;

        ImGui::SameLine();

        if (interpretAsColor) buttonColor = buttonColor * 1.2f;
        buttonColor.w = 1.0f;
        ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
        if (ImGui::Button(FormatString("%s %s", interpretAsColor ? ICON_FA_CHECK : ICON_FA_DROPLET, Localization::GetString("COLOR").c_str()).c_str())) {
            interpretAsColor = true;
        }
        buttonColor = reservedButtonColor;

        ImGui::PopStyleColor(2);
        ImGui::SameLine();


        interpreterModeSizeX = ImGui::GetCursorPosX() - beginCursorX;

        ImGui::NewLine();

        static ImVec2 childSize = ImVec2(100, 100);
        ImGui::SetCursorPos(ImVec2{
            ImGui::GetWindowSize().x / 2.0f - childSize.x / 2.0f,
            ImGui::GetWindowSize().y / 2.0f - childSize.y / 2.0f
        });

        ImGui::BeginChild("##vector3Container", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
        if (!interpretAsColor) {
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1, 0, 0, 1));
            ImGui::PlotVar(FormatString("%s %s", ICON_FA_STOPWATCH, "(x)").c_str(), vector.x);

            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0, 1, 0, 1));
            ImGui::PlotVar(FormatString("%s %s", ICON_FA_STOPWATCH, "(y)").c_str(), vector.y);

            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0, 0, 1, 1));
            ImGui::PlotVar(FormatString("%s %s", ICON_FA_STOPWATCH, "(z)").c_str(), vector.z);\

            ImGui::PopStyleColor(2);
        } else {
            float vectorPtr[4] = {
                vector.x,
                vector.y,
                vector.z
            };
            ImGui::PushItemWidth(200);
                ImGui::ColorPicker3("##colorPreview", vectorPtr, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
            ImGui::PopItemWidth();
        }
        childSize = ImGui::GetWindowSize();
        ImGui::EndChild();
    }

    void PreviewDispatchers::DispatchVector2Value(std::any& t_attribute) {
        auto vector = std::any_cast<glm::vec2>(t_attribute);

        static ImVec2 childSize = ImVec2(100, 100);
        ImGui::SetCursorPos(ImVec2{
            ImGui::GetWindowSize().x / 2.0f - childSize.x / 2.0f,
            ImGui::GetWindowSize().y / 2.0f - childSize.y / 2.0f
        });

        ImGui::BeginChild("##vector2Container", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);

        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1, 0, 0, 1));
        ImGui::PlotVar(FormatString("%s %s", ICON_FA_STOPWATCH, "(x)").c_str(), vector.x);

        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0, 1, 0, 1));
        ImGui::PlotVar(FormatString("%s %s", ICON_FA_STOPWATCH, "(y)").c_str(), vector.y);

        ImGui::PopStyleColor(2);
        childSize = ImGui::GetWindowSize();
        ImGui::EndChild();
    }

    void PreviewDispatchers::DispatchFramebufferValue(std::any& t_attribute) {
        auto framebuffer = std::any_cast<Framebuffer>(t_attribute);

        static int attachmentIndex = 0;
        if (attachmentIndex >= framebuffer.attachments.size()) attachmentIndex = framebuffer.attachments.size() - 1;
        std::any textureTarget = framebuffer.attachments[attachmentIndex];

        DispatchTextureValue(textureTarget);

        static float attachmentChooserSizeX = 0;
        ImGui::SetCursorPos({
            ImGui::GetWindowSize().x / 2.0f - attachmentChooserSizeX / 2.0f,
            0
        });
        ImGui::BeginChild("##attachmentContainer", ImVec2(attachmentChooserSizeX, 30));
        float firstCursorX = ImGui::GetCursorPosX();
        for (int i = 0; i < framebuffer.attachments.size(); i++) {
            auto buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
            buttonColor = attachmentIndex == i ? buttonColor * 1.1f : buttonColor * 0.8f;
            buttonColor.w = 1.0f;
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            if (ImGui::Button(FormatString("%s %s %i", ICON_FA_IMAGE, Localization::GetString("ATTACHMENT").c_str(), i).c_str())) {
                attachmentIndex = i;
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();
        }
        attachmentChooserSizeX = ImGui::GetCursorPosX() - firstCursorX;
        ImGui::EndChild();
    }

    void PreviewDispatchers::DispatchBoolValue(std::any& t_attribute) {
        bool value = std::any_cast<bool>(t_attribute);
        const char* text = value ? "true" : "false";
        ImGui::SetCursorPos({
            ImGui::GetWindowPos().x / 2.0f - ImGui::CalcTextSize(text).x / 2.0f,
            ImGui::GetWindowPos().y / 2.0f - ImGui::CalcTextSize(text).y / 2.0f
        });
        ImGui::Text(text);
    }
};
