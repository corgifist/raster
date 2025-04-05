#include "common/bezier_curve.h"
#include "common/colorspace.h"
#include "common/common.h"
#include "font/IconsFontAwesome5.h"
#include "gpu/gpu.h"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_stdlib.h"
#include "../../ImGui/imgui_drag.h"
#include "../../ImGui/imgui_stripes.h"
#include "overlay_dispatchers.h"
#include "string_dispatchers.h"
#include "common/transform2d.h"
#include "common/audio_samples.h"
#include "audio/audio.h"
#include "common/asset_id.h"
#include "common/ui_helpers.h"
#include "common/generic_resolution.h"
#include "common/gradient_1d.h"
#include "common/line2d.h"

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
        ImVec2 fitSize = FitRectInRect(ImVec2(128, 128), ImVec2(texture.width, texture.height));
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - fitSize.x / 2.0f);
        ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 12, 194, fitSize);

        ImGuiID imageID = ImGui::GetID("##image");
        static std::unordered_map<ImGuiID, bool> s_hoveredMap;

        if (s_hoveredMap.find(imageID) == s_hoveredMap.end()) {
            s_hoveredMap[imageID] = false;
        }

        bool& imageHovered = s_hoveredMap[imageID];

        ImVec4 tint = ImVec4(1, 1, 1, imageHovered ? 0.7f : 1.0f);
        ImGui::Image((ImTextureID) texture.handle, fitSize, ImVec2(0, 0), ImVec2(1, 1), tint, ImVec4(0, 0, 0, 0));
        imageHovered = ImGui::IsItemHovered();

        if (imageHovered && ImGui::IsItemClicked()) {
            ImGui::OpenPopup("##imagePopup");
        }

        if (ImGui::BeginPopup("##imagePopup")) {
            ImGui::Image((ImTextureID) texture.handle, FitRectInRect(ImGui::GetWindowViewport()->Size, ImVec2(texture.width, texture.height)) / 2);
            ImGui::EndPopup();
        }

        
        auto footerText = FormatString("%ix%i; %s (%s)", (int) texture.width, (int) texture.height, texture.PrecisionToString().c_str(), texture.GetShortPrecisionInfo().c_str());
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(footerText.c_str()).x / 2.0f);
        ImGui::Text("%s", footerText.c_str());
        
        auto aspectRatioText = FormatString("%s %s: %0.3f", ICON_FA_IMAGE, Localization::GetString("ASPECT_RATIO").c_str(), (float) texture.width / (float) texture.height);
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(aspectRatioText.c_str()).x / 2.0f);
        ImGui::Text("%s", aspectRatioText.c_str());
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

    void StringDispatchers::DispatchVector3Value(std::any& t_attribute) {
        auto vector = std::any_cast<glm::vec3>(t_attribute);
        ImGui::Text("%s %s: (%0.2f; %0.2f; %0.2f)", ICON_FA_CIRCLE_INFO, Localization::GetString("VALUE").c_str(), vector.x, vector.y, vector.z);
        ImGui::PushItemWidth(200);
            ImGui::ColorPicker3("##colorPreview", glm::value_ptr(vector), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
        ImGui::PopItemWidth();
    }

    void StringDispatchers::DispatchVector2Value(std::any& t_attribute) {
        auto vector = std::any_cast<glm::vec2>(t_attribute);
        ImGui::Text("%s %s: (%0.2f; %0.2f)", ICON_FA_CIRCLE_INFO, Localization::GetString("VALUE").c_str(), vector.x, vector.y);
    }

    void StringDispatchers::DispatchFramebufferValue(std::any& t_attribute) {
        auto framebuffer = std::any_cast<Framebuffer>(t_attribute);
        ImGui::Text("%s %s: %i", ICON_FA_IMAGE, Localization::GetString("ATTACHMENTS_COUNT").c_str(), (int) framebuffer.attachments.size());
        ImGui::Separator();
        ImGui::Spacing();
        int index = 0;
        for (auto& attachment : framebuffer.attachments) {
            std::any dynamicAttachment = attachment;
            ImGui::BeginChild(FormatString("##%i", (int) (uint64_t) attachment.handle).c_str(), ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
                DispatchTextureValue(dynamicAttachment);
            ImGui::EndChild();
            if (index + 1 != framebuffer.attachments.size()) {
                ImGui::Separator();
                ImGui::Spacing();
            }
            index++;
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
            ImGui::Text("%s Scale: %0.2f", ICON_FA_EXPAND, transform.scale);
            ImGui::Text("%s Anchor: %0.2f; %0.2f", ICON_FA_ANCHOR, transform.anchor.x, transform.anchor.y);
            ImGui::Text("%s Angle: %0.2f", ICON_FA_ROTATE, transform.angle);
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("##transformPreviewContainer", fitSize);
            RectBounds backgroundBounds(
                ImVec2(0, 0), 
                fitSize
            );
            ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 20, 28, fitSize);
            OverlayDispatchers::DispatchTransform2DValue(transformCopy, nullptr, -1, 0.5f, {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y});
        ImGui::EndChild();
    }

    void StringDispatchers::DispatchBoolValue(std::any& t_attribute) {
        bool value = std::any_cast<bool>(t_attribute);
        ImGui::Text("%s %s", ICON_FA_CIRCLE_INFO, value ? "true" : "false");
    }

    void StringDispatchers::DispatchAudioSamplesValue(std::any& t_attribute) {
        auto value = std::any_cast<AudioSamples>(t_attribute);
        if (!value.attachedPictures.empty()) {
            static ImVec2 coversChildSize = ImVec2{100, 50};
            ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - coversChildSize.x / 2.0f);
            if (ImGui::BeginChild("##childContainer", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
                int pictureIndex = 0;
                for (auto& picture : value.attachedPictures) {
                    std::any dynamicPicture = picture;
                    DispatchTextureValue(dynamicPicture);
                }
                coversChildSize = ImGui::GetWindowSize();
            }
            ImGui::EndChild();   
        }
        std::string sampleRateText = FormatString("%s %s: %i", ICON_FA_WAVE_SQUARE, Localization::GetString("SAMPLE_RATE").c_str(), value.sampleRate);
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(sampleRateText.c_str()).x / 2.0f);
        ImGui::Text(sampleRateText.c_str());
        UIHelpers::RenderAudioSamplesWaveform(value);
    }

    void StringDispatchers::DispatchAssetIDValue(std::any& t_attribute) {
        auto id = std::any_cast<AssetID>(t_attribute).id;
        auto assetCandidate = Workspace::GetAssetByAssetID(id);
        if (!assetCandidate.has_value()) {
            ImGui::Text("%s %s", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("ASSET_NOT_FOUND").c_str());
        } else {
            if (ImGui::BeginChild("##assetDetailsContainer", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
                auto& asset = assetCandidate.value();
                auto textureCandidate = asset->GetPreviewTexture();
                if (textureCandidate.has_value()) {
                    if (ImGui::BeginChild("##textureChild", ImVec2(0, ImGui::GetWindowSize().y), ImGuiChildFlags_AutoResizeX)) {
                        auto& texture = textureCandidate.value();
                        ImVec2 fitSize = FitRectInRect(ImVec2(150, 150), ImVec2(texture.width, texture.height));
                        ImGui::SetCursorPos(ImGui::GetWindowSize() / 2 - fitSize / 2);
                        ImGuiID imageID = ImGui::GetID("##image");
                        static std::unordered_map<ImGuiID, bool> s_hoveredMap;

                        if (s_hoveredMap.find(imageID) == s_hoveredMap.end()) {
                            s_hoveredMap[imageID] = false;
                        }

                        bool& imageHovered = s_hoveredMap[imageID];

                        ImVec4 tint = ImVec4(1, 1, 1, imageHovered ? 0.7f : 1.0f);
                        ImGui::Image((ImTextureID) texture.handle, fitSize, ImVec2(0, 0), ImVec2(1, 1), tint, ImVec4(0, 0, 0, 0));
                        imageHovered = ImGui::IsItemHovered();

                        if (imageHovered && ImGui::IsItemClicked()) {
                            ImGui::OpenPopup("##imagePopup");
                        }

                        if (ImGui::BeginPopup("##imagePopup")) {
                            ImGui::Image((ImTextureID) texture.handle, FitRectInRect(ImGui::GetWindowViewport()->Size, ImVec2(texture.width, texture.height)) / 2);
                            ImGui::EndPopup();
                        }
                    }
                    ImGui::EndChild();
                    ImGui::SameLine();
                } else {
                    ImGui::Text("%s %s", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("ASSET_HAS_NO_PREVIEW_TEXTURE").c_str());
                }
                if (ImGui::BeginChild("##assetInfo", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
                    auto implementationCandidate = Assets::GetAssetImplementationByPackageName(asset->packageName);
                    if (implementationCandidate.has_value()) {
                        ImGui::SeparatorText(FormatString("%s %s", implementationCandidate.value().description.icon.c_str(), asset->name.c_str()).c_str());
                        asset->RenderDetails();
                    } else {
                        ImGui::Text("%s %s", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("ASSET_IMPLEMENTATION_NOT_FOUND").c_str());
                    }
                }
                ImGui::EndChild();
            }
            ImGui::EndChild();
        }
    }

    void StringDispatchers::DispatchGenericResolutionValue(std::any& t_attribute) {
        auto value = std::any_cast<GenericResolution>(t_attribute);
        auto resolution = value.CalculateResolution();
        std::string screenText = FormatString("%ix%i (%0.2f)", (int) resolution.x, (int) resolution.y, (float) resolution.x / (float) resolution.y);
        ImGui::Button(screenText.c_str(), FitRectInRect(ImVec2(160, 160), ImVec2(resolution.x, resolution.y)));
    }

    void StringDispatchers::DispatchGradient1DValue(std::any& t_attribute) {
        auto value = std::any_cast<Gradient1D>(t_attribute);
        UIHelpers::RenderGradient1D(value);
    }

    void StringDispatchers::DispatchLine2DValue(std::any &t_attribute) {
        auto line = std::any_cast<Line2D>(t_attribute);
        ImGui::BeginGroup();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s P0: (%0.2f; %0.2f)", ICON_FA_UP_DOWN_LEFT_RIGHT, line.begin.x, line.begin.y);
            ImGui::SameLine();
            ImGui::ColorButton("##p0ColorIndicator", ImVec4(line.beginColor.r, line.beginColor.g, line.beginColor.b, line.beginColor.a), ImGuiColorEditFlags_AlphaPreview, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()));
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s P1: (%0.2f; %0.2f)", ICON_FA_UP_DOWN_LEFT_RIGHT, line.end.x, line.end.y);
            ImGui::SameLine();
            ImGui::ColorButton("##p1ColorIndicator", ImVec4(line.endColor.r, line.endColor.g, line.endColor.b, line.endColor.a), ImGuiColorEditFlags_AlphaPreview, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()));
        ImGui::EndGroup();
        std::any lineCopy = line;
        auto preferredResolution = Workspace::GetProject().preferredResolution;
        ImVec2 fitSize = FitRectInRect(ImVec2{128, ImGui::GetWindowSize().y}, ImVec2{preferredResolution.x, preferredResolution.y});
        ImGui::SameLine();
        ImGui::BeginChild("##transformPreviewContainer", fitSize);
            RectBounds backgroundBounds(
                ImVec2(0, 0), 
                fitSize
            );
            ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 20, 28, fitSize);
            OverlayDispatchers::DispatchLine2DValue(lineCopy, nullptr, -1, 0.5f, {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y});
        ImGui::EndChild();
    }

    void StringDispatchers::DispatchBezierCurveValue(std::any &t_attribute) {
        auto bezier = std::any_cast<BezierCurve>(t_attribute);
        ImGui::BeginGroup();
            for (int i = 0; i < bezier.points.size(); i++) {
                ImGui::Text("%s P%i: (%0.2f; %0.2f)", ICON_FA_BEZIER_CURVE, i, bezier.points[i].x, bezier.points[i].y);
            }
        ImGui::EndGroup();
        std::any lineCopy = bezier;
        auto preferredResolution = Workspace::GetProject().preferredResolution;
        ImVec2 fitSize = FitRectInRect(ImVec2{128, ImGui::GetWindowSize().y}, ImVec2{preferredResolution.x, preferredResolution.y});
        ImGui::SameLine();
        ImGui::BeginChild("##transformPreviewContainer", fitSize);
            RectBounds backgroundBounds(
                ImVec2(0, 0), 
                fitSize
            );
            ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 20, 28, fitSize);
            OverlayDispatchers::DispatchBezierCurve(lineCopy, nullptr, -1, 0.5f, {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y});
        ImGui::EndChild();
    }

    void StringDispatchers::DispatchColorspaceValue(std::any& t_attribute) {
        ImGui::Text("%s %s", ICON_FA_DROPLET, std::any_cast<Colorspace>(t_attribute).name.c_str());
    }

};