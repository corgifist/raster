#include "media_asset.h"
#include "common/composition.h"
#include "common/workspace.h"
#include "common/ui_helpers.h"
#include "raster.h"
#include <random>
#include <variant>
#include "common/asset_id.h"
#include "common/waveform_manager.h"

extern "C" {
#include <libavutil/samplefmt.h>
}

#define WAVEFORM_PRECISION 2048

namespace Raster {

    MediaAsset::MediaAsset() {
        AssetBase::Initialize();
        this->name = "Media Asset";
        this->m_formatCtxWasOpened = false;
        this->m_waveformFuture = std::nullopt;
        this->m_selectedWaveform = 0;
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
        if (m_copyFuture.has_value() && !IsFutureReady(m_copyFuture.value())) return false;
        if (m_waveformFuture.has_value() && IsFutureReady(*m_waveformFuture)) {
            m_waveformData = m_waveformFuture->get();
            m_waveformFuture = std::nullopt;
        }
        if (m_formatCtxWasOpened) return true;
        av::FormatContext formatCtx;
        if (std::filesystem::exists(absolutePath) && !std::filesystem::is_directory(absolutePath) && !formatCtx.isOpened() && !m_formatCtxWasOpened) {
            std::error_code ec;
            formatCtx.openInput(absolutePath, ec);
            if (ec) {
                std::cout << "failed to open formatCtx " << av::error2string(ec.value()) << std::endl;
            } 
            if (formatCtx.isOpened()) {
                formatCtx.findStreamInfo();
                m_cachedDuration = formatCtx.duration().seconds();

                av::Dictionary metadataDictionary(formatCtx.raw()->metadata, false);
                for (auto& pair : metadataDictionary) {
                    m_metadata.push_back({pair.key(), pair.value()});
                }

                bool containsAttachedPicture = false;
                int attachedPictureStreamIndex = -1;
                for (int i = 0; i < formatCtx.streamsCount(); i++) {
                    auto stream = formatCtx.stream(i);
                    if (stream.raw()->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                        containsAttachedPicture = true;
                        attachedPictureStreamIndex = stream.index();
                        break;
                    }
                }

                if (containsAttachedPicture) {
                    auto packet = formatCtx.readPacket();
                    if (packet && packet.streamIndex() == attachedPictureStreamIndex) {
                        auto videoDecoder = av::VideoDecoderContext(formatCtx.stream(attachedPictureStreamIndex));
                        videoDecoder.open();
                        auto inputFrame = videoDecoder.decode(packet);

                        av::VideoRescaler rescaler(
                            videoDecoder.width(), videoDecoder.height(), AV_PIX_FMT_RGB0
                        );
                        auto rescaledFrame = rescaler.rescale(inputFrame);
                        
                        if (m_attachedPicTexture.has_value()) {
                            GPU::DestroyTexture(m_attachedPicTexture.value());
                        }
                        auto attachedPicTexture = GPU::GenerateTexture(rescaledFrame.width(), rescaledFrame.height(), 4, TexturePrecision::Usual, true);
                        GPU::UpdateTexture(attachedPicTexture, 0, 0, rescaledFrame.width(), rescaledFrame.height(), 4, rescaledFrame.data());
                        GPU::GenerateMipmaps(attachedPicTexture);

                        m_attachedPicTexture = attachedPicTexture;
                    }
                }

                bool hasAnyAudioStreams = false;
                for (int i = 0; i < formatCtx.streamsCount(); i++) {
                    auto stream = formatCtx.stream(i);
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
                        info.attachedPic = attachedPic;
                        
                        m_streamInfos.push_back(info);

                        if (!m_attachedPicTexture) {
                            auto rational = stream.timeBase().getValue();
                            formatCtx.seek((int64_t) (formatCtx.duration().seconds() / 2) * (int64_t)rational.den / (int64_t) rational.num, stream.index(), AVSEEK_FLAG_BACKWARD);
                            avcodec_flush_buffers(videoDecoder.raw());
                            std::error_code ec;
                            while (true) {
                                auto pkt = formatCtx.readPacket();
                                if (!pkt) break;
                                if (pkt.streamIndex() != stream.index()) continue;
                                auto decodedFrame = videoDecoder.decode(pkt, ec);
                                if (ec && ec.value() == AVERROR(EAGAIN)) continue;
                                if (decodedFrame) {
                                    av::VideoRescaler videoRescaler(videoDecoder.width(), videoDecoder.height(), AV_PIX_FMT_RGB0);
                                    auto rescaledFrame = videoRescaler.rescale(decodedFrame);
                                    if (rescaledFrame) {
                                        if (m_attachedPicTexture.has_value()) {
                                            GPU::DestroyTexture(m_attachedPicTexture.value());
                                        }
                                        auto attachedPicTexture = GPU::GenerateTexture(rescaledFrame.width(), rescaledFrame.height(), 4, TexturePrecision::Usual, true);
                                        GPU::UpdateTexture(attachedPicTexture, 0, 0, rescaledFrame.width(), rescaledFrame.height(), 4, rescaledFrame.data());
                                        GPU::GenerateMipmaps(attachedPicTexture);

                                        m_attachedPicTexture = attachedPicTexture;
                                        break;
                                    }
                                }
                            }
                        }
                    } else if (stream.isAudio()) {
                        hasAnyAudioStreams = true;
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
                if (hasAnyAudioStreams) {
                    m_waveformFuture = std::async(std::launch::async, MediaAsset::CalculateWaveformsForPath, absolutePath);
                }
            }
            m_formatCtxWasOpened = true;
        }
        return m_formatCtxWasOpened;
    }

    AudioWaveformData MediaAsset::CalculateWaveformsForPath(std::string t_path) {
        av::FormatContext formatCtx;
        formatCtx.openInput(t_path);
        if (!formatCtx.isOpened()) return AudioWaveformData();
        AudioWaveformData waveformData;
        waveformData.streamData = std::make_shared<std::vector<std::vector<float>>>();

        formatCtx.findStreamInfo();
        for (int i = 0; i < formatCtx.streamsCount(); i++) {
            auto stream = formatCtx.stream(i);
            if (!stream.isAudio()) continue;
            auto audioDecoder = av::AudioDecoderContext(stream);
            audioDecoder.open();
            av::AudioResampler audioResampler(av::ChannelLayout(1).layout(), audioDecoder.sampleRate(), AV_SAMPLE_FMT_FLT,
                                            audioDecoder.channelLayout(), audioDecoder.sampleRate(), audioDecoder.sampleFormat());
            auto sampleRate = audioDecoder.sampleRate();
            std::vector<float> waveformSamples;
            while (true) {
                auto pkt = formatCtx.readPacket();
                if (!pkt) break;
                if (pkt.streamIndex() != stream.index()) continue;
                auto samples = audioDecoder.decode(pkt);
                if (!samples) break;
                audioResampler.push(samples);
                auto stepSamples = audioResampler.pop(WAVEFORM_PRECISION);
                if (!stepSamples) continue;
                float waveformAverage = 0.0;
                for (int i = 0; i < WAVEFORM_PRECISION; i++) {
                    float sample = std::abs(((float*) stepSamples.data())[i]);
                    sample = std::clamp(sample, 0.0f, 1.0f);
/*                     float convertedDecibel = std::abs(60 - std::min(std::abs(LinearToDecibel(std::abs(sample))), 60.0f));
                    waveformAverage = (waveformAverage + convertedDecibel) / 2.0f;  */
                    waveformAverage = (waveformAverage + sample) / 2.0f;
                }
                // waveformAverage /= 60.0f;
                waveformSamples.push_back(waveformAverage);
            }
            // DUMP_VAR(waveformSamples.size());
            waveformData.streamData->push_back(waveformSamples);
        }
        return waveformData;
    }

#define WAVEFORM_PREVIEW_HEIGHT 15

    void MediaAsset::AbstractRenderPreviewOverlay(glm::vec2 t_regionSize) {
        if (!m_waveformData) return;
        if (!m_waveformData->streamData) return;
        if (m_waveformData->streamData->empty()) return;
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        
        ImVec2 frameMin = cursorPos;
        frameMin.y += t_regionSize.y - WAVEFORM_PREVIEW_HEIGHT;
        ImVec2 frameMax = cursorPos + ImVec2(t_regionSize.x, t_regionSize.y);
        ImGui::RenderFrame(frameMin, frameMax, ImGui::GetColorU32(ImGuiCol_PopupBg));
        auto& waveform = m_waveformData->streamData->at(m_selectedWaveform);
        auto waveformStep = waveform.size() / (size_t) t_regionSize.x;
        float pixelAdvance = 0.0f;
        float advanceStep = 1.0f;
        ImVec2 clipMin = frameMin;
        clipMin.x += 1.0f;
        ImVec2 clipMax = frameMax;
        clipMax.x -= 1.0f;
        ImGui::GetWindowDrawList()->PushClipRect(clipMin, clipMax);
        for (size_t i = 0; i < waveform.size(); i += waveformStep) {
            float waveformAverage = 0.0;
            for (size_t subIndex = i; subIndex < i + waveformStep; subIndex++) {
                if (subIndex >= waveform.size()) break;
                waveformAverage = (waveformAverage + std::abs(waveform[subIndex])) / 2.0f;
            }

            ImVec2 waveformMin = frameMin;
            waveformMin.x += pixelAdvance;
            waveformMin.y += (1.0 - waveformAverage) * WAVEFORM_PREVIEW_HEIGHT;

            ImVec2 waveformMax = frameMin;
            waveformMax.x += pixelAdvance + advanceStep;
            waveformMax.y += WAVEFORM_PREVIEW_HEIGHT;

            ImGui::GetWindowDrawList()->AddRectFilled(waveformMin, waveformMax, ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
            pixelAdvance += advanceStep;
        }
        ImGui::GetWindowDrawList()->PopClipRect();
    }

    void MediaAsset::AbstractOnTimelineDrop() {
        if (!Workspace::IsProjectLoaded()) return;
        auto& project = Workspace::GetProject();
        bool hasAudioStream = false;
        bool hasVideoStream = false;
        for (auto& stream : m_streamInfos) {
            if (std::holds_alternative<VideoStreamInfo>(stream)) {
                hasVideoStream = true;
            }
            if (std::holds_alternative<AudioStreamInfo>(stream)) {
                hasAudioStream = true;
            }
        }
        
        if (hasAudioStream) {
            Composition audioComposition = Composition();
            audioComposition.beginFrame = project.currentFrame;
            audioComposition.endFrame = project.currentFrame + m_cachedDuration.value_or(1) * project.framerate;
            audioComposition.name = name;
            if (Workspace::s_colorMarks.find("Light Orange") != Workspace::s_colorMarks.end()) {
                audioComposition.colorMark = Workspace::s_colorMarks["Light Orange"];
            }

            auto assetAttributeCandidate = Attributes::InstantiateAttribute(RASTER_PACKAGED "asset_attribute");
            if (!assetAttributeCandidate) return;
            auto& assetAttribute = *assetAttributeCandidate;
            assetAttribute->name = "Audio Asset";
            assetAttribute->keyframes[0].value = AssetID(id);

            audioComposition.attributes.push_back(assetAttribute);

            auto assetAttributeNodeCandidate = Workspace::InstantiateNode(RASTER_PACKAGED "get_attribute_value");
            if (!assetAttributeNodeCandidate) return;
            auto& assetAttributeNode = *assetAttributeNodeCandidate;
            assetAttributeNode->SetAttributeValue("AttributeID", assetAttribute->id);

            auto attributeValuePinCandidate = assetAttributeNode->GetAttributePin("Value");
            if (!attributeValuePinCandidate) return;
            auto& attributeValuePin = *attributeValuePinCandidate;

            auto readAudioNodeCandidate = Workspace::InstantiateNode(RASTER_PACKAGED "decode_audio_asset");
            if (!readAudioNodeCandidate) return;
            auto& readAudioNode = *readAudioNodeCandidate;
            readAudioNode->AddInputPin("Asset");
            readAudioNode->nodePosition = glm::vec2(0, 150);
            for (auto& pin : readAudioNode->inputPins) {
                if (pin.linkedAttribute == "Asset") {
                    pin.connectedPinID = attributeValuePin.pinID;
                    break;
                }
            }

            auto samplesPinCandidate = readAudioNode->GetAttributePin("Samples");
            if (!samplesPinCandidate) return;
            auto& samplesPin = *samplesPinCandidate;

            auto exportAudioNodeCandidate = Workspace::InstantiateNode(RASTER_PACKAGED "export_to_audio_bus");
            if (!exportAudioNodeCandidate) return;
            auto& exportAudioNode = *exportAudioNodeCandidate;
            exportAudioNode->nodePosition = glm::vec2(200, 150);
            for (auto& pin : exportAudioNode->inputPins) {
                if (pin.linkedAttribute == "Samples") {
                    pin.connectedPinID = samplesPin.pinID;
                    break;
                }
            }

            audioComposition.nodes[assetAttributeNode->nodeID] = assetAttributeNode;
            audioComposition.nodes[readAudioNode->nodeID] = readAudioNode;
            audioComposition.nodes[exportAudioNode->nodeID] = exportAudioNode;
            project.compositions.push_back(audioComposition);
            WaveformManager::RequestWaveformRefresh(audioComposition.id);
        };

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
        if (acc.empty()) acc = "-";
        return acc;
    }

    std::optional<std::string> MediaAsset::AbstractGetDuration() {
        std::string acc = "";
        if (m_cachedDuration) {
            auto duration = *m_cachedDuration;
            acc = FormatString("%i:%i", (int) (duration / 60), (int) ((int) duration % 60));
        }
        if (acc.empty()) acc = "-";
        return acc;
    }

    std::optional<std::uintmax_t> MediaAsset::AbstractGetSize() {
        if (m_cachedSize.has_value()) {
            return m_cachedSize;
        }
        if (std::filesystem::exists(FormatString("%s/%s", Workspace::GetProject().path.c_str(), m_relativePath.c_str()))) {
            m_cachedSize = std::filesystem::file_size(FormatString("%s/%s", Workspace::GetProject().path.c_str(), m_relativePath.c_str()));
            return m_cachedSize;
        }
        return std::nullopt;
    }

    std::optional<std::string> MediaAsset::AbstractGetPath() {
        return m_originalPath;
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
        if (m_streamInfos.empty()) {
            ImGui::Text("%s %s", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("NO_STREAMS_FOUND").c_str());
        }
        if (!m_metadata.empty()) {
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
                    ImGui::Text("%s %s: %i Kb", ICON_FA_CIRCLE_INFO, Localization::GetString("BITRATE").c_str(), info.bitrate);
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
        }
    }

    void MediaAsset::AbstractRenderPopup() {
        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("METADATA").c_str()).c_str())) {
            for (auto& pair : m_metadata) {
                ImGui::BulletText("%s: ", pair.first.c_str());
                ImGui::SameLine();
                ImGui::Text("%s", pair.second.c_str());
                if (ImGui::IsItemClicked()) {
                    ImGui::SetClipboardText(pair.second.c_str());
                }
                ImGui::SetItemTooltip("%s %s '%s'", ICON_FA_COPY, Localization::GetString("CLICK_TO_COPY").c_str(), pair.first.c_str());
            }
            ImGui::EndMenu();
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
                "m4a", "mp3", "ogg", "wav", "flac", "aac", 
                "webm", "mkv", "flv", "mp4", "avi", "wmv","m4v",
                "amv", "mpg", "mpeg", "mp2", "3gp"
            }
        };
    }
}