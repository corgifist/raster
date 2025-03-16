#include "image_asset.h"
#include "raster.h"

namespace Raster {

    std::optional<Pipeline> ImageAsset::s_gammaPipeline;

    ImageAsset::ImageAsset() {
        AssetBase::Initialize();

        this->m_uploadID = 0;
        this->m_texture = std::nullopt;
        this->m_asyncCopy = std::nullopt;
        this->m_loader = AsyncImageLoader();

        this->m_relativePath = "";
        this->m_originalPath = "";

        if (!s_gammaPipeline.has_value()) {
            s_gammaPipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "exr_gamma_correction/shader")
            );
        }
    }

    bool ImageAsset::AbstractIsReady() {
        if (m_texture.has_value()) return true;
        if (m_asyncCopy.has_value() && !IsFutureReady(m_asyncCopy.value())) return false;
        if (!std::filesystem::exists(FormatString("%s/%s", Workspace::GetProject().path.c_str(), m_relativePath.c_str()))) return false;

        if (!m_loader.IsInitialized() && !m_uploadID) {
            m_loader = AsyncImageLoader(FormatString("%s/%s", Workspace::GetProject().path.c_str(), m_relativePath.c_str()));
        }

        if (m_loader.IsInitialized() && m_loader.IsReady()) {
            auto imageCandidate = m_loader.Get();
            if (imageCandidate.has_value()) {
                auto& image = imageCandidate.value();

                TexturePrecision precision = TexturePrecision::Usual;
                if (image->precision == ImagePrecision::Half) precision = TexturePrecision::Half;
                if (image->precision == ImagePrecision::Full) precision = TexturePrecision::Full;

                if (!m_uploadID) {
                    m_uploadID = AsyncUpload::GenerateTextureFromImage(image);
                    m_loader = AsyncImageLoader();   
                }
            }
        }
        if (AsyncUpload::IsUploadReady(m_uploadID)) {
            auto info = AsyncUpload::GetUpload(m_uploadID);
            m_texture = info.texture;

            // DUMP_VAR(GetExtension(m_relativePath));
            // RASTER_LOG("UPLOAD IS READY");
            if (s_gammaPipeline.has_value() && GetExtension(m_relativePath) == ".exr") {
                // RASTER_LOG("EXR GAMMA CORRECTION");
                auto& pipeline = s_gammaPipeline.value();
                Texture gammaTexture = GPU::GenerateTexture(info.texture.width, info.texture.height, info.texture.channels, info.texture.precision);
                Framebuffer gammaFbo = GPU::GenerateFramebuffer(info.texture.width, info.texture.height, {gammaTexture});

                GPU::BindFramebuffer(gammaFbo);
                GPU::BindPipeline(pipeline);
                GPU::ClearFramebuffer(0, 0, 0, 0);

                GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(info.texture.width, info.texture.height));
                GPU::BindTextureToShader(pipeline.fragment, "uTexture", info.texture, 0);

                GPU::DrawArrays(3);

                GPU::BindFramebuffer(std::nullopt);
                GPU::DestroyTexture(info.texture);
                GPU::DestroyFramebuffer(gammaFbo);
                m_texture = gammaTexture;
            }

            
            AsyncUpload::DestroyUpload(m_uploadID);
        }

        return false;
    }

    std::optional<Texture> ImageAsset::AbstractGetPreviewTexture() {
        return m_texture;
    }

    std::optional<std::uintmax_t> ImageAsset::AbstractGetSize() {
        if (m_cachedSize.has_value()) return m_cachedSize;
        if (std::filesystem::exists(FormatString("%s/%s", Workspace::GetProject().path.c_str(), m_relativePath.c_str()))) {
            m_cachedSize = std::filesystem::file_size(FormatString("%s/%s", Workspace::GetProject().path.c_str(), m_relativePath.c_str()));
            return m_cachedSize;
        }
        return std::nullopt;
    }

    std::optional<std::string> ImageAsset::AbstractGetResolution() {
        if (m_texture.has_value()) {
            auto& texture = m_texture.value();
            return FormatString("%ix%i", (int) texture.width, (int) texture.height);
        }
        return std::nullopt;
    }

    std::optional<std::string> ImageAsset::AbstractGetPath() {
        return m_originalPath;
    }

    void ImageAsset::AbstractImport(std::string t_path) {
        this->m_originalPath = t_path;
        std::string relativePath = FormatString("%i%s", id, GetExtension(t_path).c_str());
        std::string absolutePath = FormatString("%s/%s", Workspace::GetProject().path.c_str(), relativePath.c_str());
        this->m_relativePath = relativePath;
        this->m_asyncCopy = std::async(std::launch::async, [t_path, absolutePath]() {
            std::filesystem::copy(t_path, absolutePath);
            return true;
        });
        this->name = GetBaseName(t_path);
    }

    Json ImageAsset::AbstractSerialize() {
        return {
            {"RelativePath", m_relativePath},
            {"OriginalPath", m_originalPath}
        };
    }

    void ImageAsset::AbstractLoad(Json t_data) {
        this->m_relativePath = t_data["RelativePath"];
        this->m_originalPath = t_data["OriginalPath"];
    }

    void ImageAsset::AbstractRenderDetails() {
        if (!IsReady()) {
            ImGui::Text("%s %s", ICON_FA_SPINNER, Localization::GetString("IMAGE_IS_NOT_READY_FOR_USE_YET").c_str());
        } else {
            auto& texture = m_texture.value();
            ImGui::Text("%s %s: %ix%i", ICON_FA_EXPAND, Localization::GetString("IMAGE_RESOLUTION").c_str(), (int) texture.width, (int) texture.height);
            ImGui::Text("%s %s: %0.2f", ICON_FA_IMAGE, Localization::GetString("ASPECT_RATIO").c_str(), (float) texture.width / (float) texture.height);
            ImGui::Text("%s %s: %i", ICON_FA_DROPLET, Localization::GetString("NUMBER_OF_CHANNELS").c_str(), texture.channels);
        }
    }

    void ImageAsset::AbstractDelete() {
        if (std::filesystem::exists(FormatString("%s/%s", Workspace::GetProject().path.c_str(), m_relativePath.c_str()))) {
            std::filesystem::remove(FormatString("%s/%s", Workspace::GetProject().path.c_str(), m_relativePath.c_str()));
        }

        auto textureCandidate = AbstractGetPreviewTexture();
        if (textureCandidate.has_value()) {
            GPU::DestroyTexture(textureCandidate.value());
        }
    }
};

extern "C" {
    Raster::AbstractAsset SpawnAsset() {
        return (Raster::AbstractAsset) std::make_shared<Raster::ImageAsset>();
    }

    Raster::AssetDescription GetDescription() {
        return Raster::AssetDescription{
            .prettyName = "Image Asset",
            .packageName = RASTER_PACKAGED "image_asset",
            .icon = ICON_FA_IMAGE,
            .extensions = Raster::ImageLoader::GetSupportedExtensions()
        };
    }
}