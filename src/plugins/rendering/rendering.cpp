#include "rendering.h"
#include "common/configuration.h"
#include "common/generic_video_decoder.h"
#include "common/localization.h"
#include "common/plugin_base.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"
#include "raster.h"
#include "../../ImGui/imgui.h"
#include "gpu/gpu.h"

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
        ImGui::DragInt("##videoCacheSize", &videoCacheSize, 1, 64, 3145728);
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