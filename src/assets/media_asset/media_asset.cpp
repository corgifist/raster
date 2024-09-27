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

                av::Dictionary metadataDictionary(m_formatCtx.raw()->metadata, false);
                for (auto& pair : metadataDictionary) {
                    m_metadata.push_back({pair.key(), pair.value()});
                }

                for (int i = 0; i < m_formatCtx.streamsCount(); i++) {
                    auto stream = m_formatCtx.stream(i);
                    bool attachedPic = stream.raw()->disposition & AV_DISPOSITION_ATTACHED_PIC;
                    if (stream.isVideo()) {
                        auto videoDecoder = av::VideoDecoderContext(stream);
                        videoDecoder.open(Codec());

                        VideoStreamInfo info;
                        info.width = videoDecoder.width();
                        info.height = videoDecoder.height();
                        info.codecName = videoDecoder.codec().longName();
                        info.bitrate = videoDecoder.bitRate();
                        info.pixelFormatName = videoDecoder.pixelFormat().name();
                        m_streamInfos.push_back(info);

                        if (attachedPic) {
                            auto packet = m_formatCtx.readPacket();
                            if (packet && packet.streamIndex() == i) {
                                auto inputFrame = videoDecoder.decode(packet);

                                av::VideoRescaler rescaler(
                                    videoDecoder.width(), videoDecoder.height(), AV_PIX_FMT_RGB0
                                );
                                auto rescaledFrame = rescaler.rescale(inputFrame);
                                
                                if (m_attachedPicTexture.has_value()) {
                                    GPU::DestroyTexture(m_attachedPicTexture.value());
                                }
                                auto attachedPicTexture = GPU::GenerateTexture(rescaledFrame.width(), rescaledFrame.height(), 4);
                                GPU::UpdateTexture(attachedPicTexture, 0, 0, rescaledFrame.width(), rescaledFrame.height(), 4, rescaledFrame.data());

                                m_attachedPicTexture = attachedPicTexture;
                            }
                        }
                    } else if (stream.isAudio()) {
                        auto audioDecoder = av::AudioDecoderContext(stream);
                        audioDecoder.open(Codec());
                        AudioStreamInfo info;
                        info.sampleRate = audioDecoder.sampleRate();
                        info.sampleFormatName = audioDecoder.sampleFormat().name();
                        info.codecName = audioDecoder.codec().longName();
                        info.bitrate = audioDecoder.bitRate();
                        
                        
                        m_streamInfos.push_back(info);
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
        std::string acc = "";
        int streamIndex = 0;
        for (auto& stream : m_streamInfos) {
            bool last = streamIndex + 1 == m_streamInfos.size();
            std::string separator = (last ? "" : " | ");
            if (std::holds_alternative<VideoStreamInfo>(stream)) {
                auto info = std::get<VideoStreamInfo>(stream);
                acc += FormatString("%ix%i", info.width, info.height) + separator;
            } else if (std::holds_alternative<AudioStreamInfo>(stream)) {
                auto info = std::get<AudioStreamInfo>(stream);
                acc += FormatString("%i Hz", info.sampleRate) + separator;
            }
            streamIndex++;
        }
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
        if (m_formatCtx.isOpened()) {
            if (m_streamInfos.empty()) {
                ImGui::Text("%s %s", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("NO_STREAMS_FOUND").c_str());
            }
            if (!m_metadata.empty()) {
                std::string metadataPopupID = FormatString("##metadataPopup%i", id);
                std::string streamsPopupID = FormatString("##streamPopup%i", id);
                if (ImGui::Button(FormatString("%s %s (%i %s)", ICON_FA_LIST, Localization::GetString("OPEN_STREAMS_INFO").c_str(), (int) m_streamInfos.size(), Localization::GetString("ENTRIES").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    ImGui::OpenPopup(streamsPopupID.c_str());
                }
                if (ImGui::BeginPopup(streamsPopupID.c_str())) {
                    ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LIST, name.c_str()).c_str());
                    int streamIndex = 0;
                    for (auto& stream : m_streamInfos) {
                        std::string icon = ICON_FA_QUESTION;
                        std::string streamName = "";
                        if (std::holds_alternative<VideoStreamInfo>(stream)) {
                            icon = ICON_FA_VIDEO;
                            streamName = Localization::GetString("VIDEO_STREAM");
                        } else if (std::holds_alternative<AudioStreamInfo>(stream)) {
                            icon = ICON_FA_VOLUME_HIGH;
                            streamName = Localization::GetString("AUDIO_STREAM");
                        }

                        ImGui::Text("%s %s #%i", icon.c_str(), streamName.c_str(), streamIndex++);
                        ImGui::Indent();

                        if (std::holds_alternative<VideoStreamInfo>(stream)) {
                            auto info = std::get<VideoStreamInfo>(stream);
                            ImGui::Text("%s %s: %ix%i", ICON_FA_EXPAND, Localization::GetString("RESOLUTION").c_str(), info.width, info.height);
                            ImGui::Text("%s %s: %i", ICON_FA_CIRCLE_INFO, Localization::GetString("BITRATE").c_str(), info.bitrate);
                            ImGui::Text("%s %s: %s", ICON_FA_DROPLET, Localization::GetString("PIXEL_FORMAT").c_str(), info.pixelFormatName.c_str());
                            ImGui::Text("%s %s: %s", ICON_FA_AUDIO_DESCRIPTION, Localization::GetString("CODEC_NAME").c_str(), info.codecName.c_str());
                        } else if (std::holds_alternative<AudioStreamInfo>(stream)) {
                            auto info = std::get<AudioStreamInfo>(stream);
                            ImGui::Text("%s %s: %i", ICON_FA_FILE_WAVEFORM, Localization::GetString("SAMPLE_RATE").c_str(), info.sampleRate);
                            ImGui::Text("%s %s: %i Kb", ICON_FA_CIRCLE_INFO, Localization::GetString("BITRATE").c_str(), info.bitrate / 1024);
                            ImGui::Text("%s %s: %s", ICON_FA_CIRCLE_INFO, Localization::GetString("SAMPLE_FORMAT").c_str(), info.sampleFormatName.c_str());
                            ImGui::Text("%s %s: %s", ICON_FA_AUDIO_DESCRIPTION, Localization::GetString("CODEC_NAME").c_str(), info.codecName.c_str());
                        }
                        ImGui::Unindent();
                    }
                    ImGui::EndPopup();
                }
                if (ImGui::Button(FormatString("%s %s (%i %s)", ICON_FA_LIST, Localization::GetString("OPEN_METADATA").c_str(), (int) m_metadata.size(), Localization::GetString("ENTRIES").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    ImGui::OpenPopup(metadataPopupID.c_str());
                }
                if (ImGui::BeginPopup(metadataPopupID.c_str())) {
                    ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LIST, name.c_str()).c_str());
                    if (ImGui::BeginChild("##metadataList", ImVec2(0, 300), ImGuiChildFlags_AutoResizeX)) {
                        for (auto& pair : m_metadata) {
                            ImGui::BulletText("%s: ", pair.first.c_str());
                            ImGui::SameLine();
                            ImGui::Text("%s", pair.second.c_str());
                            if (ImGui::IsItemClicked()) {
                                ImGui::SetClipboardText(pair.second.c_str());
                            }
                            ImGui::SetItemTooltip("%s %s '%s'", ICON_FA_COPY, Localization::GetString("CLICK_TO_COPY").c_str(), pair.first.c_str());
                        }
                    }
                    ImGui::EndChild();
                    ImGui::EndPopup();
                }
            }
        }
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