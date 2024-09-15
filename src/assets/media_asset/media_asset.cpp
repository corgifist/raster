#include "media_asset.h"
#include "common/workspace.h"

namespace Raster {
    MediaAsset::MediaAsset() {
        AssetBase::Initialize();
        this->name = "Media Asset";
        this->m_formatCtxWasOpened = false;
    }

    void MediaAsset::AbstractImport(std::string t_path) {
        this->m_originalPath = t_path;
        std::string relativePath = FormatString("%i%s", id, GetExtension(t_path).c_str());
        std::string absolutePath = FormatString("%s/%s", Workspace::GetProject().path.c_str(), relativePath.c_str());
        this->m_relativePath = relativePath;
        m_copyFuture = std::async(std::launch::async, [t_path, absolutePath]() {
            std::filesystem::copy(t_path, absolutePath);
            return true;
        });

        this->name = GetBaseName(t_path);
    }

    void MediaAsset::AbstractDelete() {
        std::string absolutePath = FormatString("%s/%s", Workspace::GetProject().path.c_str(), m_relativePath.c_str());
        if (std::filesystem::exists(absolutePath) && !std::filesystem::is_directory(absolutePath)) {
            std::filesystem::remove(absolutePath);
        }
    }

    bool MediaAsset::AbstractIsReady() {
        std::string absolutePath = FormatString("%s/%s", Workspace::GetProject().path.c_str(), m_relativePath.c_str());
        if (std::filesystem::exists(absolutePath) && !std::filesystem::is_directory(absolutePath) && !m_formatCtx.isOpened() && !m_formatCtxWasOpened) {
            std::error_code ec;
            m_formatCtx.openInput(absolutePath, ec);
            if (ec) {
                std::cout << "failed to open formatCtx! " << av::error2string(ec.value()) << std::endl;
            } 
            if (m_formatCtx.isOpened()) {
                m_formatCtx.findStreamInfo();
                for (int i = 0; i < m_formatCtx.streamsCount(); i++) {
                    auto stream = m_formatCtx.stream(i);
                    if (stream.isVideo() && stream.raw()->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                        auto videoDecoder = av::VideoDecoderContext(stream);
                        std::cout << name << ": attached pic decoder: " << videoDecoder.raw()->codec->long_name << std::endl;
                        videoDecoder.open(Codec());

                        auto packet = m_formatCtx.readPacket();
                        if (packet && packet.streamIndex() == i) {
                            auto inputFrame = videoDecoder.decode(packet);

                            av::VideoRescaler rescaler(
                                videoDecoder.width(), videoDecoder.height(), AV_PIX_FMT_RGB0
                            );
                            auto rescaledFrame = rescaler.rescale(inputFrame);
                            
                            auto attachedPicTexture = GPU::GenerateTexture(rescaledFrame.width(), rescaledFrame.height(), 4);
                            GPU::UpdateTexture(attachedPicTexture, 0, 0, rescaledFrame.width(), rescaledFrame.height(), 4, rescaledFrame.data());

                            m_attachedPicTexture = attachedPicTexture;
                        }
                    }
                }
            }
            m_formatCtxWasOpened = true;
        }
        return m_formatCtxWasOpened;
    }

    std::optional<Texture> MediaAsset::AbstractGetPreviewTexture() {
        return  m_attachedPicTexture;
    }

    std::optional<std::string> MediaAsset::AbstractGetResolution() {
        return std::nullopt;
    }

    std::optional<std::uintmax_t> MediaAsset::AbstractGetSize() {
        if (std::filesystem::exists(FormatString("%s/%s", Workspace::GetProject().path.c_str(), m_relativePath.c_str()))) {
            return std::filesystem::file_size(FormatString("%s/%s", Workspace::GetProject().path.c_str(), m_relativePath.c_str()));
        }
        return std::nullopt;
    }

    std::optional<std::string> MediaAsset::AbstractGetPath() {
        if (std::filesystem::exists(m_originalPath) && !std::filesystem::is_directory(m_originalPath)) {
            return m_originalPath;
        }
        return std::nullopt;
    }

    Json MediaAsset::AbstractSerialize() {
        return {
            {"OriginalPath", m_originalPath},
            {"RelativePath", m_relativePath}
        };
    }

    void MediaAsset::AbstractLoad(Json t_data) {
        this->m_originalPath = t_data["OriginalPath"];
        this->m_relativePath = t_data["RelativePath"];
    }

    void MediaAsset::AbstractRenderDetails() {

    }
};

extern "C" {
    Raster::AbstractAsset SpawnAsset() {
        return (Raster::AbstractAsset) std::make_shared<Raster::MediaAsset>();
    }

    Raster::AssetDescription GetDescription() {
        return Raster::AssetDescription{
            .prettyName = "Media Asset",
            .packageName = RASTER_PACKAGED "media_asset",
            .icon = ICON_FA_IMAGES,
            .extensions = {
                "m4a", "mp3", "ogg", "wav", "flac", "aac"
            }
        };
    }
}