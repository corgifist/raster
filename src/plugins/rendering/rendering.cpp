#include "rendering.h"
#include "common/configuration.h"
#include "common/dispatchers.h"
#include "common/generic_video_decoder.h"
#include "common/localization.h"
#include "common/plugin_base.h"
#include "common/workspace.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"
#include "raster.h"
#include "../../ImGui/imgui.h"
#include "gpu/gpu.h"
#include "image/image.h"
#include "common/dispatchers.h"
#include "common/convolution_kernel.h"
#include "common/rendering.h"
#include "../../ImGui/imgui_stdlib.h"

namespace Raster {
    std::string RenderingPlugin::AbstractName() {
        return "Rendering";
    }

    std::string RenderingPlugin::AbstractDescription() {
        return "Performs configuration of all rendering subsystems";
    }

    std::string RenderingPlugin::AbstractIcon() {
        return ICON_FA_VIDEO;
    }

    std::string RenderingPlugin::AbstractPackageName() {
        return RASTER_PACKAGED "rendering";
    }

    void RenderingPlugin::AbstractRenderProperties() {
        static auto s_ramAmount = GetRamAmount();

        auto& pluginData = GetPluginData();
        ImGui::PushFont(Font::s_denseFont);
            ImGui::SetWindowFontScale(1.4f);
            ImGui::Text("%s %s:", ICON_FA_IMAGE, Localization::GetString("RENDERER_INFO").c_str());
            ImGui::SetWindowFontScale(1.0f);
        ImGui::PopFont();

        ImGui::Indent();
            auto& info = GPU::info;
            ImGui::Text("%s %s: %s", ICON_FA_GEARS, Localization::GetString("RENDERER_NAME").c_str(), info.renderer.c_str());
            ImGui::Text("%s %s: %s", ICON_FA_GEARS, Localization::GetString("RENDERER_VERSION").c_str(), info.version.c_str());
            ImGui::Text("%s %s: %ix%i", ICON_FA_EXPAND, Localization::GetString("MAX_TEXTURE_SIZE").c_str(), info.maxTextureSize, info.maxTextureSize);
            ImGui::Text("%s %s: %ix%i", ICON_FA_EXPAND, Localization::GetString("MAX_VIEWPORT_SIZE").c_str(), info.maxViewportX, info.maxViewportY);
        ImGui::Unindent();

        bool videoCachingEnabled = pluginData["VideoCachingEnabled"];
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s %s", ICON_FA_VIDEO, Localization::GetString("VIDEO_CACHING_ENABLED").c_str());
        ImGui::SameLine();
        ImGui::Checkbox("##videoCachingEnabled", &videoCachingEnabled);
        pluginData["VideoCachingEnabled"] = videoCachingEnabled;

        if (!videoCachingEnabled) ImGui::BeginDisabled();
        int videoCacheSize = pluginData["VideoCacheSize"];
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s %s", ICON_FA_BOX_OPEN, Localization::GetString("MAX_VIDEO_CACHE_SIZE").c_str());
        ImGui::SameLine();
        ImGui::DragInt("##videoCacheSize", &videoCacheSize, 1, 16, 3145728);
        pluginData["VideoCacheSize"] = videoCacheSize;

        ImGui::Text(FormatString("%s %s", ICON_FA_CIRCLE_INFO, Localization::GetString("VIDEO_CACHE_SIZE_APPROXIMATION").c_str()).c_str(), videoCacheSize, videoCacheSize / 6);

        if (!videoCachingEnabled) ImGui::EndDisabled();
    }

    void RenderingPlugin::AbstractOnEarlyInitialization() {
        auto& pluginData = GetPluginData();
        auto defaultConfiguration = GetDefaultConfiguration();
        for (auto& defaultItem : defaultConfiguration.items()) {
            if (!pluginData.contains(defaultItem.key())) {
                pluginData[defaultItem.key()] = defaultItem.value();
            }
        }
        if (pluginData["VideoCachingEnabled"]) {
            GenericVideoDecoder::InitializeCache(pluginData["VideoCacheSize"]);
        }

    }

    static ImVec2 FitRectInRect(ImVec2 dst, ImVec2 src) {
        float scale = std::min(dst.x / src.x, dst.y / src.y);
        return ImVec2{src.x * scale, src.y * scale};
    }

    static void DispatchStringConvolutionKernel(std::any& t_attribute) {
        auto kernel = std::any_cast<ConvolutionKernel>(t_attribute);
        ImGui::BeginGroup();
            ImGui::Text("%s %s: %0.2f", ICON_FA_XMARK, Localization::GetString("MULTIPLIER").c_str(), kernel.multiplier);
            ImGui::PushItemWidth(170);
                ImGui::DragFloat3("##matrix0", glm::value_ptr(kernel.kernel[0]));
                ImGui::DragFloat3("##matrix1", glm::value_ptr(kernel.kernel[1]));
                ImGui::DragFloat3("##matrix2", glm::value_ptr(kernel.kernel[2]));
            ImGui::PopItemWidth();
        ImGui::EndGroup();
        if (GPU::s_imageConvolutionPreviewTexture.handle) {
            auto& previewTexture = GPU::s_imageConvolutionPreviewTexture;
            ImGui::SameLine();
            ImGui::BeginGroup();
                static Framebuffer s_previewFramebuffer;
                if (!s_previewFramebuffer.handle) {
                    s_previewFramebuffer = GPU::GenerateFramebuffer(previewTexture.width, previewTexture.height, {GPU::GenerateTexture(previewTexture.width, previewTexture.height, 3)});
                }
                static std::optional<glm::mat3> s_lastRenderedKernel = std::nullopt;
                if (!s_lastRenderedKernel || (s_lastRenderedKernel && *s_lastRenderedKernel != kernel.GetKernel())) {
                    GPU::BindPipeline(GPU::s_kernelPreviewPipeline);
                    GPU::BindFramebuffer(s_previewFramebuffer);
                    GPU::ClearFramebuffer(0, 0, 0, 0);
                    GPU::SetShaderUniform(GPU::s_kernelPreviewPipeline.fragment, "uResolution", glm::vec2(s_previewFramebuffer.width, s_previewFramebuffer.height));
                    GPU::SetShaderUniform(GPU::s_kernelPreviewPipeline.fragment, "uKernel", kernel.GetKernel());
                    GPU::BindTextureToShader(GPU::s_kernelPreviewPipeline.fragment, "uBase", previewTexture, 0);
                    GPU::DrawArrays(3);
                    s_lastRenderedKernel = kernel.GetKernel();
                }
                auto fitSize = FitRectInRect(ImVec2(100, 100), ImVec2(previewTexture.width, previewTexture.height));
                ImGui::SetCursorPosY(ImGui::GetWindowSize().y / 2.0f - fitSize.y / 2.0f);
                ImGui::Image((ImTextureID) s_previewFramebuffer.attachments[0].handle, fitSize);
            ImGui::EndGroup();
        }
    }

    static void DispatchConvolutionKernelAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata) {
        auto kernel = std::any_cast<ConvolutionKernel>(t_value);
        bool isItemEdited = false;
        if (GPU::s_imageConvolutionPreviewTexture.handle) {
            ImVec2 dstSize = ImVec2(256, 256);
            auto fitSize = FitRectInRect(dstSize, ImVec2(GPU::s_imageConvolutionPreviewTexture.width, GPU::s_imageConvolutionPreviewTexture.height));
            ImVec2 iFitSize = ImVec2(fitSize.x, fitSize.y);
            ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - iFitSize.x / 2.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::BeginChild("##previewContainer", iFitSize, ImGuiChildFlags_Border);
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
                auto fitSize = FitRectInRect(dstSize, ImVec2(previewTexture.width, previewTexture.height));
                ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - fitSize.x / 2.0f);
                ImGui::Image((ImTextureID) s_previewFramebuffer.attachments[0].handle, fitSize);
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }

        ImGui::BeginChild("##kernelDrags", ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, 0));
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s %s ", ICON_FA_XMARK, Localization::GetString("MULTIPLIER").c_str());
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat("##multiplierDrag", &kernel.multiplier, 0.01f);
            if (ImGui::IsItemEdited()) isItemEdited = true;
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragFloat3("##kernel0", glm::value_ptr(kernel.kernel[0]), 1);
                if (ImGui::IsItemEdited()) isItemEdited = true;
                ImGui::DragFloat3("##kernel1", glm::value_ptr(kernel.kernel[1]), 1);
                if (ImGui::IsItemEdited()) isItemEdited = true;
                ImGui::DragFloat3("##kernel2", glm::value_ptr(kernel.kernel[2]), 1);
                if (ImGui::IsItemEdited()) isItemEdited = true;
            ImGui::PopItemWidth();
            static bool s_searchFocused = false;
            if (ImGui::Button(FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("KERNEL_PRESETS").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                ImGui::OpenPopup("##kernelPresets");
            } 
            if (ImGui::BeginPopup("##kernelPresets")) {
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
                            ImGui::CloseCurrentPopup();
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
                ImGui::EndPopup();
            } else s_searchFocused = false;
        ImGui::EndChild();

        if (isItemEdited) Rendering::ForceRenderFrame();
        t_value = kernel;
    }

    static void DispatchPreviewConvolutionKernelAttribute(std::any& t_attribute) {
        static ImVec2 s_childSize = ImVec2(300, 300);

        ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - s_childSize / 2.0f);
        if (ImGui::BeginChild("##kernelContainer", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
            auto kernel = std::any_cast<ConvolutionKernel>(t_attribute);
            if (GPU::s_imageConvolutionPreviewTexture.handle) {
                ImVec2 dstSize = ImVec2(256, 256);
                auto fitSize = FitRectInRect(dstSize, ImVec2(GPU::s_imageConvolutionPreviewTexture.width, GPU::s_imageConvolutionPreviewTexture.height));
                ImVec2 iFitSize = ImVec2(fitSize.x, fitSize.y);
                ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - iFitSize.x / 2.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                ImGui::BeginChild("##previewContainer", iFitSize, ImGuiChildFlags_Border);
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
                    auto fitSize = FitRectInRect(dstSize, ImVec2(previewTexture.width, previewTexture.height));
                    ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - fitSize.x / 2.0f);
                    ImGui::Image((ImTextureID) s_previewFramebuffer.attachments[0].handle, fitSize);
                }
                ImGui::EndChild();
                ImGui::PopStyleVar();
            }

            ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - 200 / 2.0f);
            ImGui::BeginChild("##kernelDrags", ImVec2(200, 0), ImGuiChildFlags_AutoResizeY);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s %s ", ICON_FA_XMARK, Localization::GetString("MULTIPLIER").c_str());
                ImGui::SameLine();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragFloat("##multiplierDrag", &kernel.multiplier, 0.01f);
                ImGui::PopItemWidth();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::DragFloat3("##kernel0", glm::value_ptr(kernel.kernel[0]), 1);
                    ImGui::DragFloat3("##kernel1", glm::value_ptr(kernel.kernel[1]), 1);
                    ImGui::DragFloat3("##kernel2", glm::value_ptr(kernel.kernel[2]), 1);
                ImGui::PopItemWidth();
            ImGui::EndChild();

            s_childSize = ImGui::GetWindowSize();
        }
        ImGui::EndChild();
    }

    void RenderingPlugin::AbstractOnWorkspaceInitialization() {
        Dispatchers::s_stringDispatchers[std::type_index(typeid(ConvolutionKernel))] = DispatchStringConvolutionKernel;
        Dispatchers::s_propertyDispatchers[std::type_index(typeid(ConvolutionKernel))] = DispatchConvolutionKernelAttribute;
        Dispatchers::s_previewDispatchers[std::type_index(typeid(ConvolutionKernel))] = DispatchPreviewConvolutionKernelAttribute;
    }

    Json RenderingPlugin::GetDefaultConfiguration() {
        return {
            {"VideoCachingEnabled", true},
            {"VideoCacheSize", GetRamAmount() / 6291456} // 1024 * 1024 * 6 = 6291456
        };
    }
};

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractPlugin SpawnPlugin() {
        return RASTER_SPAWN_PLUGIN(Raster::RenderingPlugin);
    }
}