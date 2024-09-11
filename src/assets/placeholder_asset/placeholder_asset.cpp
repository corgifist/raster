#include "placeholder_asset.h"

namespace Raster {

    std::optional<Pipeline> PlaceholderAsset::s_smptePipeline;

    PlaceholderAsset::PlaceholderAsset() {
        AssetBase::Initialize();

        if (!s_smptePipeline.has_value()) {
            s_smptePipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "smpte_pattern/shader")
            );
        }

        this->name = "SMPTE Pattern";

        this->m_resolution = {512, 512};
    }

    bool PlaceholderAsset::AbstractIsReady() {
        return true;
    }

    std::optional<Texture> PlaceholderAsset::AbstractGetPreviewTexture() {
        if (s_smptePipeline.has_value() && (!m_texture.has_value() || (m_texture.has_value() && (m_texture.value().width != m_resolution.x || m_texture.value().height != m_resolution.y)))) {
            if (m_texture.has_value()) {
                GPU::DestroyTexture(m_texture.value());
            }
            auto& pipeline = s_smptePipeline.value();

            Texture smpteTexture = GPU::GenerateTexture(m_resolution.x, m_resolution.y, 3, TexturePrecision::Usual);
            Framebuffer smpteFramebuffer = GPU::GenerateFramebuffer(m_resolution.x, m_resolution.y, {smpteTexture});

            GPU::BindFramebuffer(smpteFramebuffer);
            GPU::BindPipeline(pipeline);
            GPU::ClearFramebuffer(0, 0, 0, 1);

            GPU::SetShaderUniform(pipeline.fragment, "uResolution", m_resolution);
            GPU::DrawArrays(3);

            GPU::DestroyFramebuffer(smpteFramebuffer);
            m_texture = smpteTexture;
        }
        return m_texture;
    }

    std::optional<std::string> PlaceholderAsset::AbstractGetResolution() {
        if (m_texture.has_value()) {
            auto& texture = m_texture.value();
            return FormatString("%ix%i", (int) texture.width, (int) texture.height);
        }
        return std::nullopt;
    }

    Json PlaceholderAsset::AbstractSerialize() {
        return {
            {"Resolution", {m_resolution.x, m_resolution.y}}
        };
    }

    void PlaceholderAsset::AbstractLoad(Json t_data) {
        this->m_resolution = {t_data["Resolution"][0], t_data["Resolution"][1]};
    }

    void PlaceholderAsset::AbstractRenderDetails() {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s %s", ICON_FA_EXPAND, Localization::GetString("RESOLUTION").c_str());
        ImGui::SameLine();
        ImGui::DragFloat2("##dragResolution", glm::value_ptr(m_resolution), 1, 0, 0, "%0.0f");
    }
};

extern "C" {
    Raster::AbstractAsset SpawnAsset() {
        return (Raster::AbstractAsset) std::make_shared<Raster::PlaceholderAsset>();
    }

    Raster::AssetDescription GetDescription() {
        return Raster::AssetDescription{
            .prettyName = "Placeholder Asset",
            .packageName = RASTER_PACKAGED "placeholder_asset",
            .icon = ICON_FA_FOLDER_CLOSED,
            .extensions = {}
        };
    }
}